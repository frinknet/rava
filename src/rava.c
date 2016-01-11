#include "rava.h"

void ray_pump_cb(uv_async_t* async)
{
  (void)async;
}

void ray_pump(void)
{
  uv_async_send(&RAY_PUMP);
}


#include "rava_state.h"
#include "rava_buffer.h"
#include "rava_fiber.h"
#include "rava_timer.h"
#include "rava_stream.h"
#include "rava_pipe.h"
#include "rava_tcp.h"
#include "rava_udp.h"
#include "rava_fs.h"

#include "rava_state.c"
#include "rava_buffer.c"
#include "rava_fiber.c"
#include "rava_timer.c"
#include "rava_stream.c"
#include "rava_pipe.c"
#include "rava_tcp.c"
#include "rava_udp.c"
#include "rava_fs.c"

static luaL_Reg ray_funcs[] = {
  {"self",      ray_self},
  {"fiber",     ray_fiber_new},
  {"timer",     ray_timer_new},
  {"pipe",      ray_pipe_new},
  {"tcp",       ray_tcp_new},
  {"udp",       ray_udp_new},
  {"run",       ray_run},
  {NULL,        NULL}
};

LUALIB_API int luaopen_ray(lua_State* L) {

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  lua_settop(L, 0);

  luaL_newmetatable(L, "ray.fiber");
  luaL_register(L, NULL, ray_fiber_meths);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "ray.timer");
  luaL_register(L, NULL, ray_timer_meths);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "ray.pipe");
  luaL_register(L, NULL, ray_pipe_meths);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "ray.tcp");
  luaL_register(L, NULL, ray_tcp_meths);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "ray.udp");
  luaL_register(L, NULL, ray_udp_meths);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "ray.file");
  luaL_register(L, NULL, ray_file_meths);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  uv_async_init(uv_default_loop(), &RAY_PUMP, ray_pump_cb);
  uv_unref((uv_handle_t*)&RAY_PUMP);

  RAY_MAIN = (ray_fiber_t*)malloc(sizeof(ray_fiber_t));
  RAY_MAIN->L   = L;
  RAY_MAIN->ref = LUA_NOREF;

  RAY_MAIN->flags = RAY_STARTED;

  QUEUE_INIT(&RAY_MAIN->queue);
  QUEUE_INIT(&RAY_MAIN->fiber_queue);

  uv_default_loop()->data = RAY_MAIN;

  luaL_register(L, "ray", ray_funcs);
  return 1;
}

