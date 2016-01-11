#include "rava_pipe.h"
#include "rava_agent.h"
#include "rava_stream.h"

int ray_pipe_new(lua_State* L)
{
  int ipc = luaL_optint(L, 1, 0);
  ray_agent_t* self = ray_agent_new(L);
  luaL_getmetatable(L, "ray.pipe");
  lua_setmetatable(L, -2);
  uv_pipe_init(uv_default_loop(), &self->h.pipe, ipc);
  return 1;
}

int ray_pipe_accept(lua_State* L)
{
  lua_settop(L, 1);
  ray_pipe_new(L);
  return ray_accept(L);
}

int ray_pipe_bind(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  const char* name = luaL_checkstring(L, 2);

  int rc = uv_pipe_bind(&self->h.pipe, name);
  if (rc) return ray_push_error(L, rc);

  lua_settop(L, 1);
  return 1;
}

int ray_pipe_open(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  uv_file fh = luaL_checkint(L, 2);

  int rc = uv_pipe_open(&self->h.pipe, fh);
  if (rc) return ray_push_error(L, rc);

  lua_settop(L, 1);
  return 1;
}

int ray_pipe_connect(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  ray_fiber_t* curr = ray_current(L);
  const char* name = luaL_checkstring(L, 2);

  uv_pipe_connect(&curr->r.connect, &self->h.pipe, name, ray_connect_cb);

  assert(QUEUE_EMPTY(&curr->queue));
  QUEUE_INSERT_TAIL(&self->fiber_queue, &curr->queue);

  return ray_suspend(curr);
}

static luaL_Reg ray_pipe_meths[] = {
  {"read",      ray_read},
  {"write",     ray_write},
  {"close",     ray_close},
  {"listen",    ray_listen},
  {"shutdown",  ray_shutdown},
  {"open",      ray_pipe_open},
  {"accept",    ray_pipe_accept},
  {"bind",      ray_pipe_bind},
  {"connect",   ray_pipe_connect},
  {"__gc",      ray_agent_free},
  {NULL,        NULL}
};

