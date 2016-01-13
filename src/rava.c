#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
LUALIB_API int luaopen_rava(lua_State *L)
{

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  int i;
  uv_loop_t*    loop;
  rava_state_t*  curr;
  rava_object_t* stdfh;

  lua_settop(L, 0);

  /* register decoders */
  lua_pushcfunction(L, ravaL_lib_decoder);
  lua_setfield(L, LUA_REGISTRYINDEX, "rava:lib:decoder");

  /* rava */
  ravaL_new_module(L, "rava", rava_funcs);

  /* rava.thread */
  ravaL_new_module(L, "rava_thread", rava_thread_funcs);
  lua_setfield(L, -2, "thread");
  ravaL_new_class(L, RAVA_THREAD_T, rava_thread_meths);
  lua_pop(L, 1);

  if (!MAIN_INITIALIZED) {
    ravaL_thread_init_main(L);
    lua_pop(L, 1);
  }

  /* rava.fiber */
  ravaL_new_module(L, "rava_fiber", rava_fiber_funcs);

  /* borrow coroutine.yield (fast on LJ2) */
  lua_getglobal(L, "coroutine");
  lua_getfield(L, -1, "yield");
  lua_setfield(L, -3, "yield");
  lua_pop(L, 1); /* coroutine */

  lua_setfield(L, -2, "fiber");

  ravaL_new_class(L, RAVA_FIBER_T, rava_fiber_meths);
  lua_pop(L, 1);

  /* rava.serialize */
  ravaL_new_module(L, "rava_serialize", rava_serialize_funcs);
  lua_setfield(L, -2, "serialize");

  /* rava.timer */
  ravaL_new_module(L, "rava_timer", rava_timer_funcs);
  lua_setfield(L, -2, "timer");
  ravaL_new_class(L, RAVA_TIMER_T, rava_timer_meths);
  lua_pop(L, 1);

  /* rava.idle */
  ravaL_new_module(L, "rava_idle", rava_idle_funcs);
  lua_setfield(L, -2, "idle");
  ravaL_new_class(L, RAVA_IDLE_T, rava_idle_meths);
  lua_pop(L, 1);

  /* rava.fs */
  ravaL_new_module(L, "rava_system_fs", rava_system_fs_funcs);
  lua_setfield(L, -2, "fs");
  ravaL_new_class(L, RAVA_FILE_T, rava_system_file_meths);
  lua_pop(L, 1);

  /* rava.pipe */
  ravaL_new_module(L, "rava_pipe", rava_pipe_funcs);
  lua_setfield(L, -2, "pipe");
  ravaL_new_class(L, RAVA_PIPE_T, rava_stream_meths);
  luaL_register(L, NULL, rava_pipe_meths);
  lua_pop(L, 1);

  /* rava.std{in,out,err} */
  if (!MAIN_INITIALIZED) {
    MAIN_INITIALIZED = 1;
    loop = ravaL_event_loop(L);
    curr = ravaL_state_self(L);

    const char* stdfhs[] = { "stdin", "stdout", "stderr" };
    for (i = 0; i < 3; i++) {
#ifdef WIN32
      const uv_file fh = GetStdHandle(i == 0 ? STD_INPUT_HANDLE
       : (i == 1 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE));
#else
      const uv_file fh = i;
#endif
      stdfh = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));
      luaL_getmetatable(L, RAVA_PIPE_T);
      lua_setmetatable(L, -2);
      ravaL_object_init(curr, stdfh);
      uv_pipe_init(loop, &stdfh->h.pipe, 0);
      uv_pipe_open(&stdfh->h.pipe, fh);
      lua_pushvalue(L, -1);
      lua_setfield(L, LUA_REGISTRYINDEX, stdfhs[i]);
      lua_setfield(L, -2, stdfhs[i]);
    }
  }

  /* rava.net */
  ravaL_new_module(L, "rava_net", rava_net_funcs);
  lua_setfield(L, -2, "net");
  ravaL_new_class(L, RAVA_NET_TCP_T, rava_stream_meths);
  luaL_register(L, NULL, rava_net_tcp_meths);
  lua_pop(L, 1);

  /* rava.process */
  ravaL_new_module(L, "rava_process", rava_process_funcs);
  lua_setfield(L, -2, "process");
  ravaL_new_class(L, RAVA_PROCESS_T, rava_process_meths);
  lua_pop(L, 1);

  lua_settop(L, 1);
  return 1;
}

#ifdef __cplusplus
}
#endif
