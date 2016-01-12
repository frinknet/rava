#include "rava_fiber.h"

int ray_fiber_new(lua_State* L)
{
  ray_fiber_t* self = lua_newuserdata(L, sizeof(ray_fiber_t));

  luaL_getmetatable(L, "ray.fiber");
  lua_setmetatable(L, -2);
  lua_insert(L, 1);

  lua_pushvalue(L, 1);
  self->ref = luaL_ref(L, LUA_REGISTRYINDEX);

  self->L     = lua_newthread(L);
  self->L_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  self->flags = 0;

  QUEUE_INIT(&self->queue);
  QUEUE_INIT(&self->fiber_queue);

  uv_idle_init(uv_default_loop(), &self->hook);

  lua_xmove(L, self->L, lua_gettop(L) - 1);
  ray_ready(self);

  return 1;
}

int ray_fiber_ready(lua_State* L)
{
  ray_fiber_t* self = (ray_fiber_t*)lua_touserdata(L, 1);

  ray_ready(self);

  return 1;
}

int ray_fiber_suspend(lua_State* L)
{
  ray_fiber_t* self = (ray_fiber_t*)lua_touserdata(L, 1);

  return ray_suspend(self);
}

int ray_fiber_join(lua_State* L)
{
  ray_fiber_t* self = (ray_fiber_t*)lua_touserdata(L, 1);
  ray_fiber_t* curr = ray_current(L);

  assert(QUEUE_EMPTY(&curr->queue));
  QUEUE_INSERT_TAIL(&self->fiber_queue, &curr->queue);
  ray_ready(self);

  return ray_suspend(curr);
}

static luaL_Reg ray_fiber_funcs[] = {
  {"create",   ray_fiber_new},
  {NULL,        NULL}
};

static luaL_Reg ray_fiber_meths[] = {
  {"join",      ray_fiber_join},
  {"ready",     ray_fiber_ready},
  {"suspend",   ray_fiber_suspend},
  {NULL,        NULL}
};

LUA_API int LUA_MODULE(RAY_MODULE_FIBER, lua_State* L)
{
  rayL_module(L, RAY_MODULE_FIBER, ray_fiber_funcs);

  /* borrow coroutine.yield (fast on LJ2) */
  lua_getglobal(L, "coroutine");
  lua_getfield(L, -1, "yield");
  lua_setfield(L, -3, "yield");
  lua_pop(L, 1); /* coroutine */

  rayL_class(L, RAY_CLASS_FIBER, ray_fiber_meths);
  lua_pop(L, 1);

  ray_init_main(L);

  return 1;
}
