#include "rava.h"
#include "rava_common.h"

LUA_API int luaopen_rava_process(lua_State *L);
LUA_API int luaopen_rava_socket(lua_State *L);
LUA_API int luaopen_rava_system(lua_State *L);

LUA_API int luaopen_rava(lua_State *L)
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

	luaopen_rava_process(L);
	luaopen_rava_socket(L);
	luaopen_rava_system(L);

  lua_settop(L, 1);

  return 1;
}
