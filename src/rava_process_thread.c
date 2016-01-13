#include "rava.h"

void ravaL_thread_ready(rava_thread_t* self)
{
  if (!(self->flags & RAVA_FREADY)) {
    TRACE("SET READY\n");

    self->flags |= RAVA_FREADY;

    uv_async_send(&self->async);
  }
}

int ravaL_thread_yield(rava_thread_t* self, int narg)
{
  TRACE("calling uv_run_once\n");

  uv_run_once(self->loop);

  TRACE("done\n");

  if (narg == LUA_MULTRET) {
    narg = lua_gettop(self->L);
  }

  return narg;
}

int ravaL_thread_suspend(rava_thread_t* self)
{
  if (self->flags & RAVA_FREADY) {
    self->flags &= ~RAVA_FREADY;

    int active = 0;

    do {
      TRACE("loop top\n");

      ravaL_thread_loop(self);

      active = uv_run_once(self->loop);

      TRACE("uv_run_once returned, active: %i\n", active);

      if (self->flags & RAVA_FREADY) {
        TRACE("main ready, breaking\n");

        break;
      }
    } while (active);

    TRACE("back in main\n");

    /* nothing left to do, back in main */
    self->flags |= RAVA_FREADY;
  }

  return lua_gettop(self->L);
}

int ravaL_thread_resume(rava_thread_t* self, int narg)
{
  /* interrupt the uv_run_once loop in ravaL_thread_schedule */
  TRACE("resuming...\n");

  ravaL_thread_ready(self);

  if (narg) {
    /* pass arguments from current state to self */
    rava_state_t* curr = self->curr;

    if (narg == LUA_MULTRET) {
      narg = lua_gettop(curr->L);
    }

    /* but only if it is not to ourselves */
    if (curr != (rava_state_t*)self) {
      assert(lua_gettop(curr->L) >= narg);
      lua_checkstack(self->L, narg);
      lua_xmove(curr->L, self->L, narg);
    }
  } else {
    lua_settop(self->L, 0); /* ??? */
  }

  return narg;
}

void ravaL_thread_enqueue(rava_thread_t* self, rava_fiber_t* fiber)
{
  int need_async = QUEUE_EMPTY(&self->rouse);
  QUEUE_INSERT_TAIL(&self->rouse, &fiber->queue);

  if (need_async) {
    TRACE("need async\n");

    /* interrupt the event loop (the sequence of these two calls matters) */
    uv_async_send(&self->async);

    /* make sure we loop at least once more */
    uv_ref((uv_handle_t*)&self->async);
  }
}

rava_thread_t* ravaL_thread_self(lua_State* L)
{
  rava_state_t* self = ravaL_state_self(L);

  if (self->type == RAVA_TTHREAD) {
    return (rava_thread_t*)self;
  } else {
    while (self->type != RAVA_TTHREAD) self = self->outer;

    return (rava_thread_t*)self;
  }
}

int ravaL_thread_once(rava_thread_t* self)
{
  if (!QUEUE_EMPTY(&self->rouse)) {
    QUEUE* q;
    rava_fiber_t* fiber;

    q = QUEUE_HEAD(&self->rouse);
    fiber = QUEUE_DATA(q, rava_fiber_t, queue);

    QUEUE_REMOVE(q);

    TRACE("[%p] rouse fiber: %p\n", self, fiber);

    if (fiber->flags & RAVA_FDEAD) {
      TRACE("[%p] fiber is dead: %p\n", self, fiber);

      luaL_error(self->L, "cannot resume a dead fiber");
    } else {
      int stat, narg;
      narg = lua_gettop(fiber->L);

      if (!(fiber->flags & RAVA_FSTART)) {
        /* first entry, ignore function arg */
        fiber->flags |= RAVA_FSTART;

        --narg;
      }

      self->curr = (rava_state_t*)fiber;

      TRACE("[%p] calling lua_resume on: %p\n", self, fiber);

      stat = lua_resume(fiber->L, narg);

      TRACE("resume returned\n");

      self->curr = (rava_state_t*)self;

      switch (stat) {
        case LUA_YIELD:
          TRACE("[%p] seen LUA_YIELD\n", self);

          /* if called via coroutine.yield() then we're still in the queue */
          if (fiber->flags & RAVA_FREADY) {
            TRACE("%p is still ready, back in the queue\n", fiber);

            QUEUE_INSERT_TAIL(&self->rouse, &fiber->queue);
          }

          break;
        case 0: {
          /* normal exit, wake up joining states */
          int i, narg;

          narg = lua_gettop(fiber->L);

          TRACE("[%p] normal exit - fiber: %p, narg: %i\n", self, fiber, narg);

          QUEUE* q;
          rava_state_t* s;

          while (!QUEUE_EMPTY(&fiber->rouse)) {
            q = QUEUE_HEAD(&fiber->rouse);
            s = QUEUE_DATA(q, rava_state_t, join);

            QUEUE_REMOVE(q);

            TRACE("calling ravaL_state_ready(%p)\n", s);

            ravaL_state_ready(s);

            if (s->type == RAVA_TFIBER) {
              lua_checkstack(fiber->L, 1);
              lua_checkstack(s->L, narg);

              for (i = 1; i <= narg; i++) {
                lua_pushvalue(fiber->L, i);
                lua_xmove(fiber->L, s->L, 1);
              }
            }
          }

          TRACE("closing fiber %p\n", fiber);

          ravaL_fiber_close(fiber);

          break;
        }
        default:
          TRACE("ERROR: in fiber\n");

          lua_pushvalue(fiber->L, -1);  /* error message */
          lua_xmove(fiber->L, self->L, 1);
          ravaL_fiber_close(fiber);
          lua_error(self->L);
      }
    }
  }

  return !QUEUE_EMPTY(&self->rouse);
}

int ravaL_thread_loop(rava_thread_t* self)
{
  while (ravaL_thread_once(self));

  return 0;
}

static void _async_cb(uv_async_t* handle)
{
  TRACE("interrupt loop\n");

  (void)handle;
}

void ravaL_thread_init_main(lua_State* L)
{
  rava_thread_t* self = (rava_thread_t*)lua_newuserdata(L, sizeof(rava_thread_t));

  luaL_getmetatable(L, RAVA_THREAD_T);
  lua_setmetatable(L, -2);

  self->type  = RAVA_TTHREAD;
  self->flags = RAVA_FREADY;
  self->loop  = uv_default_loop();
  self->curr  = (rava_state_t*)self;
  self->L     = L;
  self->outer = (rava_state_t*)self;
  self->data  = NULL;
  self->tid   = (uv_thread_t)uv_thread_self();

  QUEUE_INIT(&self->rouse);

  uv_async_init(self->loop, &self->async, _async_cb);
  uv_unref((uv_handle_t*)&self->async);

  lua_pushthread(L);
  lua_pushvalue(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);
}

static void _thread_enter(void* arg)
{
  rava_thread_t* self = (rava_thread_t*)arg;

  ravaL_serialize_decode(self->L);
  lua_remove(self->L, 1);

  luaL_checktype(self->L, 1, LUA_TFUNCTION);
  lua_pushcfunction(self->L, ravaL_traceback);
  lua_insert(self->L, 1);

  int nargs = lua_gettop(self->L) - 2;
  int r = lua_pcall(self->L, nargs, LUA_MULTRET, 1);

  lua_remove(self->L, 1); /* traceback */

  if (r) { /* error */
    lua_pushboolean(self->L, 0);
    lua_insert(self->L, 1);
    ravaL_thread_ready(self);
    luaL_error(self->L, lua_tostring(self->L, -1));
  } else {
    lua_pushboolean(self->L, 1);
    lua_insert(self->L, 1);
  }

  self->flags |= RAVA_FDEAD;
}

rava_thread_t* ravaL_thread_create(rava_state_t* outer, int narg)
{
  lua_State* L = outer->L;
  int base;

  /* ..., func, arg1, ..., argN */
  base = lua_gettop(L) - narg + 1;
  rava_thread_t* self = (rava_thread_t*)lua_newuserdata(L, sizeof(rava_thread_t));

  luaL_getmetatable(L, RAVA_THREAD_T);
  lua_setmetatable(L, -2);
  lua_insert(L, base++);

  self->type  = RAVA_TTHREAD;
  self->flags = RAVA_FREADY;
  self->loop  = uv_loop_new();
  self->curr  = (rava_state_t*)self;
  self->L     = luaL_newstate();
  self->outer = outer;
  self->data  = NULL;

  QUEUE_INIT(&self->rouse);

  uv_async_init(self->loop, &self->async, _async_cb);
  uv_unref((uv_handle_t*)&self->async);

  luaL_openlibs(self->L);
  luaopen_rava(self->L);

  lua_settop(self->L, 0);
  ravaL_serialize_encode(L, narg);
  luaL_checktype(L, -1, LUA_TSTRING);
  lua_xmove(L, self->L, 1);

  /* keep a reference for reverse lookup in child */
  lua_pushthread(self->L);
  lua_pushlightuserdata(self->L, (void*)self);
  lua_rawset(self->L, LUA_REGISTRYINDEX);

  uv_thread_create(&self->tid, _thread_enter, self);

  /* inserted udata below function, so now just udata on top */
  TRACE("HERE TOP: %i, base: %i\n", lua_gettop(L), base);

  lua_settop(L, base - 1);

  return self;
}

/* Lua API */
static int rava_new_thread(lua_State* L)
{
  rava_state_t* outer = ravaL_state_self(L);

  int narg = lua_gettop(L);

  ravaL_thread_create(outer, narg);

  return 1;
}

static int rava_thread_join(lua_State* L)
{
  rava_thread_t* self = (rava_thread_t*)luaL_checkudata(L, 1, RAVA_THREAD_T);
  rava_thread_t* curr = ravaL_thread_self(L);

  ravaL_thread_ready(self);
  ravaL_thread_suspend(curr);
  uv_thread_join(&self->tid); /* TODO: use async instead, this blocks hard */

  lua_settop(L, 0);

  int nret = lua_gettop(self->L);

  ravaL_serialize_encode(self->L, nret);
  lua_xmove(self->L, L, 1);
  ravaL_serialize_decode(L);

  return nret;
}

static int rava_thread_free(lua_State* L)
{
  rava_thread_t* self = lua_touserdata(L, 1);

  TRACE("free thread\n");

  uv_loop_delete(self->loop);

  TRACE("ok\n");

  return 1;
}

static int rava_thread_tostring(lua_State* L) {
  rava_thread_t* self = (rava_thread_t*)luaL_checkudata(L, 1, RAVA_THREAD_T);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_THREAD_T, self);

  return 1;
}

luaL_Reg rava_thread_funcs[] = {
  {"spawn",     rava_new_thread},
  {NULL,        NULL}
};

luaL_Reg rava_thread_meths[] = {
  {"join",      rava_thread_join},
  {"__gc",      rava_thread_free},
  {"__tostring",rava_thread_tostring},
  {NULL,        NULL}
};
