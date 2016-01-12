#include "rava_libs.h"

#include "rava_agent.c"
#include "rava_buffer.c"
#include "rava_fiber.c"
#include "rava_timer.c"
#include "rava_stream.c"
#include "rava_state.c"
#include "rava_pipe.c"
#include "rava_tcp.c"
#include "rava_udp.c"
#include "rava_fs.c"

void ray_pump_cb(uv_async_t* async)
{
  (void)async;
}

void ray_pump(void)
{
  uv_async_send(&RAY_PUMP);
}


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

LUA_API int LUA_MODULE(RAY_MODULE, lua_State* L)
{

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  lua_settop(L, 0);

  LUA_MODULE(RAY_MODULE_FS, L)
  LUA_MODULE(RAY_MODULE_SYSTEM, L)
  LUA_MODULE(RAY_MODULE_FIBER, L)
  LUA_MODULE(RAY_MODULE_PIPE, L)
  LUA_MODULE(RAY_MODULE_TCP, L)
  LUA_MODULE(RAY_MODULE_UDP, L)

  uv_async_init(uv_default_loop(), &RAY_PUMP, ray_pump_cb);
  uv_unref((uv_handle_t*)&RAY_PUMP);

  RAY_MAIN = (ray_fiber_t*)malloc(sizeof(ray_fiber_t));
  RAY_MAIN->L   = L;
  RAY_MAIN->ref = LUA_NOREF;

  RAY_MAIN->flags = RAY_STARTED;

  QUEUE_INIT(&RAY_MAIN->queue);
  QUEUE_INIT(&RAY_MAIN->fiber_queue);

  uv_default_loop()->data = RAY_MAIN;

  luaL_register(L, RAY_MODULE, ray_funcs);

  return 1;
}
