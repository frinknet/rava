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

static luaL_Reg ray_fiber_meths[] = {
  {"join",      ray_fiber_join},
  {"ready",     ray_fiber_ready},
  {"suspend",   ray_fiber_suspend},
  {NULL,        NULL}
};


