#include "rava_state.h"

ray_fiber_t* ray_current(lua_State* L)
{
  (void)L;

  return (ray_fiber_t*)uv_default_loop()->data;
}

int ray_suspend(ray_fiber_t* fiber)
{
  uv_idle_stop(&fiber->hook);
  fiber->flags &= ~RAY_RUNNING;

  return lua_yield(fiber->L, lua_gettop(fiber->L));
}

void ray_sched_cb(uv_idle_t* idle)
{
  ray_fiber_t* self = container_of(idle, ray_fiber_t, hook);
  ray_resume(self, LUA_MULTRET);
}

int ray_ready(ray_fiber_t* fiber)
{
  uv_idle_start(&fiber->hook, ray_sched_cb);
  return 1;
}

int ray_finish(ray_fiber_t* self)
{
  TRACE("finish: %p\n", self);
  while (!QUEUE_EMPTY(&self->fiber_queue)) {
    int i, n;
    QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
    ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

    QUEUE_REMOVE(q);
    QUEUE_INIT(q);

    n = lua_gettop(self->L);
    for (i = 1; i <= n; i++) {
      lua_pushvalue(self->L, i);
    } 

    lua_xmove(self->L, f->L, n);
    ray_resume(f, n);
  }

  uv_idle_stop(&self->hook);

  if (!QUEUE_EMPTY(&self->queue)) {
    QUEUE_REMOVE(&self->queue);
    QUEUE_INIT(&self->queue);
  }

  lua_settop(self->L, 0);
  luaL_unref(self->L, LUA_REGISTRYINDEX, self->L_ref);
  luaL_unref(self->L, LUA_REGISTRYINDEX, self->ref);

  return 1;
}

int ray_resume(ray_fiber_t* fiber, int narg)
{
  TRACE("resuming: %p\n", fiber);
  if (fiber->flags & RAY_STARTED && lua_status(fiber->L) != LUA_YIELD) {
    TRACE("refusing to resume dead coroutine\n");
    return -1;
  }
  if (fiber == RAY_MAIN) {
    TRACE("refusing to resume RAY_MAIN\n");
    return -1;
  }

  if (narg == LUA_MULTRET) {
    narg = lua_gettop(fiber->L);
  }
  if (!(fiber->flags & RAY_STARTED)) {
    fiber->flags |= RAY_STARTED;
    --narg;
  }
  uv_loop_t* loop = uv_default_loop();
  void* prev = loop->data;
  loop->data = fiber;
  assert(fiber != RAY_MAIN);
  fiber->flags |= RAY_RUNNING;
  int rc = lua_resume(fiber->L, narg);
  loop->data = prev;

  switch (rc) {
    case LUA_YIELD: {
      if (fiber->flags & RAY_RUNNING) {
        ray_ready(fiber);
      }
      break;
    }
    case 0: {
      ray_finish(fiber);
      break;
    }
    default: {
      return luaL_error(RAY_MAIN->L, lua_tostring(fiber->L, -1));
    }
  }
  return 1;
}

int ray_push_error(lua_State* L, uv_errno_t err)
{
  lua_settop(L, 0);
  lua_pushnil(L);
  lua_pushstring(L, uv_err_name(err));

  return 2;
}

int ray_run(lua_State* L)
{
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  return 0;
}

int ray_self(lua_State* L)
{
  ray_fiber_t* self = (ray_fiber_t*)(uv_default_loop()->data);
  if (self) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
  }
  else {
    lua_pushnil(L);
  }
  return 1;
}
