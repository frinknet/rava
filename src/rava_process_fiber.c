#include "rava.h"

void ravaL_fiber_close(rava_fiber_t* fiber) {
  if (fiber->flags & RAVA_FDEAD) return;

  lua_pushthread(fiber->L);
  lua_pushnil(fiber->L);
  lua_settable(fiber->L, LUA_REGISTRYINDEX);

  fiber->flags |= RAVA_FDEAD;
}

void ravaL_fiber_ready(rava_fiber_t* fiber) {
  if (!(fiber->flags & RAVA_FREADY)) {
    TRACE("insert fiber %p into queue of %p\n", fiber, ravaL_thread_self(fiber->L));
    fiber->flags |= RAVA_FREADY;
    ravaL_thread_enqueue(ravaL_thread_self(fiber->L), fiber);
  }
}
int ravaL_fiber_yield(rava_fiber_t* self, int narg) {
  ravaL_fiber_ready(self);
  return lua_yield(self->L, narg);
}
int ravaL_fiber_suspend(rava_fiber_t* self) {
  TRACE("FIBER SUSPEND - READY? %i\n", self->flags & RAVA_FREADY);
  if (self->flags & RAVA_FREADY) {
    self->flags &= ~RAVA_FREADY;
    if (!ravaL_state_is_active((rava_state_t*)self)) {
      ngx_queue_remove(&self->queue);
    }
    TRACE("about to yield...\n");
    return lua_yield(self->L, lua_gettop(self->L)); /* keep our stack */
  }
  return 0;
}
int ravaL_fiber_resume(rava_fiber_t* self, int narg) {
  ravaL_fiber_ready(self);
  return lua_resume(self->L, narg);
}

rava_fiber_t* ravaL_fiber_create(rava_state_t* outer, int narg) {
  TRACE("spawn fiber as child of: %p\n", outer);

  rava_fiber_t* self;
  lua_State* L = outer->L;

  int base = lua_gettop(L) - narg + 1;
  luaL_checktype(L, base, LUA_TFUNCTION);

  lua_State* L1 = lua_newthread(L);
  lua_insert(L, base);                             /* [thread, func, ...] */

  lua_checkstack(L1, narg);
  lua_xmove(L, L1, narg);                          /* [thread] */

  self = (rava_fiber_t*)lua_newuserdata(L, sizeof(rava_fiber_t));
  luaL_getmetatable(L, RAVA_FIBER_T);               /* [thread, fiber, meta] */
  lua_setmetatable(L, -2);                         /* [thread, fiber] */

  lua_pushvalue(L, -1);                            /* [thread, fiber, fiber] */
  lua_insert(L, base);                             /* [fiber, thread, fiber] */
  lua_rawset(L, LUA_REGISTRYINDEX);                /* [fiber] */

  while (outer->type != RAVA_TTHREAD) outer = outer->outer;

  self->type  = RAVA_TFIBER;
  self->outer = outer;
  self->L     = L1;
  self->flags = 0;
  self->data  = NULL;
  self->loop  = outer->loop;

  /* fibers waiting for us to finish */
  ngx_queue_init(&self->rouse);
  ngx_queue_init(&self->queue);

  return self;
}

/* Lua API */
static int rava_new_fiber(lua_State* L) {
  rava_state_t* outer = ravaL_state_self(L);
  ravaL_fiber_create(outer, lua_gettop(L));
  assert(lua_gettop(L) == 1);
  return 1;
}

int ravaL_state_xcopy(rava_state_t* a, rava_state_t* b) {
  int i, narg;
  narg = lua_gettop(a->L);
  lua_checkstack(a->L, 1);
  lua_checkstack(b->L, narg);
  for (i = 1; i <= narg; i++) {
    lua_pushvalue(a->L, i);
    lua_xmove(a->L, b->L, 1);
  }
  return narg;
}

static int rava_fiber_join(lua_State* L) {
  rava_fiber_t* self = (rava_fiber_t*)luaL_checkudata(L, 1, RAVA_FIBER_T);
  rava_state_t* curr = (rava_state_t*)ravaL_state_self(L);
  TRACE("joining fiber[%p], from [%p]\n", self, curr);
  assert((rava_state_t*)self != curr);
  if (self->flags & RAVA_FDEAD) {
    /* seen join after termination */
    TRACE("join after termination\n");
    return ravaL_state_xcopy((rava_state_t*)self, curr);
  }
  ngx_queue_insert_tail(&self->rouse, &curr->join);
  ravaL_fiber_ready(self);
  TRACE("calling ravaL_state_suspend on %p\n", curr);
  if (curr->type == RAVA_TFIBER) {
    return ravaL_state_suspend(curr);
  }
  else {
    ravaL_state_suspend(curr);
    return ravaL_state_xcopy((rava_state_t*)self, curr);
  }
}

static int rava_fiber_ready(lua_State* L) {
  rava_fiber_t* self = (rava_fiber_t*)lua_touserdata(L, 1);
  ravaL_fiber_ready(self);
  return 1;
}
static int rava_fiber_free(lua_State* L) {
  rava_fiber_t* self = (rava_fiber_t*)lua_touserdata(L, 1);
  if (self->data) free(self->data);
  return 1;
}
static int rava_fiber_tostring(lua_State* L) {
  rava_fiber_t* self = (rava_fiber_t*)lua_touserdata(L, 1);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_FIBER_T, self);
  return 1;
}

luaL_Reg rava_fiber_funcs[] = {
  {"create",    rava_new_fiber},
  {NULL,        NULL}
};

luaL_Reg rava_fiber_meths[] = {
  {"join",      rava_fiber_join},
  {"ready",     rava_fiber_ready},
  {"__gc",      rava_fiber_free},
  {"__tostring",rava_fiber_tostring},
  {NULL,        NULL}
};


