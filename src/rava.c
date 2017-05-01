#include "rava.h"
#include "rava_core.h"

LUA_API int luaopen_rava_fs(lua_State *L);
LUA_API int luaopen_rava_process(lua_State *L);
LUA_API int luaopen_rava_socket(lua_State *L);
LUA_API int luaopen_rava_system(lua_State *L);

static int rava_serialize(lua_State* L)
{
  return ravaL_serialize_encode(L, lua_gettop(L));
}

static int rava_unserialize(lua_State* L)
{
  return ravaL_serialize_decode(L);
}

luaL_Reg rava_funcs[] = {
  {"serialize",   rava_serialize},
  {"unserialize", rava_unserialize},
  {NULL,          NULL}
};

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

	ravaL_module(L, "rava", rava_funcs);

	luaopen_rava_fs(L);
	lua_setfield(L, -2, "fs");

	luaopen_rava_socket(L);
	lua_setfield(L, -2, "socket");

	luaopen_rava_system(L);
	lua_setfield(L, -2, "system");

	luaopen_rava_process(L);
	lua_setfield(L, -2, "process");

	return 1;
}
