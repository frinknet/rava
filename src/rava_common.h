#include "rava_config.h"

int ravaL_traceback(lua_State* L);
int ravaL_require(lua_State* L, const char* path);
int ravaL_lib_decoder(lua_State* L);
int ravaL_lib_encoder(lua_State* L);
int ravaL_module(lua_State* L, const char* name, luaL_Reg* funcs);
int ravaL_class(lua_State* L, const char* name, luaL_Reg* meths);
int ravaL_extend(lua_State* L, const char* base, const char* name, luaL_Reg* meths);
void* ravaL_checkudata(lua_State* L, int idx, const char* name);
void ravaL_dump_stack(lua_State* L);
