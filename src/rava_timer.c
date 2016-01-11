#include "rava_timer.h"
#include "rava_agent.h"
#include "rava_stream.h"

int ray_timer_new(lua_State* L)
{
  ray_agent_t* self = ray_agent_new(L);
  luaL_getmetatable(L, "ray.timer");
  lua_setmetatable(L, -2);

  int rc = uv_timer_init(uv_default_loop(), &self->h.timer);
  if (rc) return ray_push_error(L, rc);

  return 1;
}

void ray_timer_cb(uv_timer_t* timer)
{
  ray_agent_t* self = container_of(timer, ray_agent_t, h);
  QUEUE* tail = QUEUE_PREV(&self->fiber_queue);

  while (!QUEUE_EMPTY(&self->fiber_queue)) {
    QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
    ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

    QUEUE_REMOVE(q);
    QUEUE_INIT(q);

    lua_settop(f->L, 0);
    lua_pushboolean(f->L, 1);
    ray_resume(f, 1);

    if (q == tail) break;
  }
}

int ray_timer_start(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.timer");
  int64_t timeout = luaL_optlong(L, 2, 0L);
  int64_t repeat  = luaL_optlong(L, 3, 0L);

  int rc = uv_timer_start(&self->h.timer, ray_timer_cb, timeout, repeat);
  if (rc) return ray_push_error(L, rc);

  return 1;
}

int ray_timer_stop(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.timer");

  int rc = uv_timer_stop(&self->h.timer);
  if (rc) return ray_push_error(L, rc);

  return 1;
}

int ray_timer_wait(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.timer");
  ray_fiber_t* curr = ray_current(L);
  assert(QUEUE_EMPTY(&curr->queue));
  QUEUE_INSERT_TAIL(&self->fiber_queue, &curr->queue);
  return ray_suspend(curr);
}

static luaL_Reg ray_timer_meths[] = {
  {"close",     ray_close},
  {"start",     ray_timer_start},
  {"stop",      ray_timer_stop},
  {"wait",      ray_timer_wait},
  {"__gc",      ray_agent_free},
  {NULL,        NULL}
};

