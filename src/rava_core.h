#include "rava_config.h"
#include "rava_core_object.h"
#include "rava_core_serialize.h"
#include "rava_core_state.h"
#include "rava_core_stream.h"

int ravaL_traceback(lua_State* L);
int ravaL_lib_decoder(lua_State* L);
int ravaL_lib_encoder(lua_State* L);
uv_loop_t* ravaL_event_loop(lua_State* L);
int ravaL_module(lua_State* L, const char* name, luaL_Reg* funcs);
int ravaL_class(lua_State* L, const char* name, luaL_Reg* meths);
int ravaL_extend(lua_State* L, const char* base, const char* name, luaL_Reg* meths);
void* ravaL_checkudata(lua_State* L, int idx, const char* name);
void ravaL_dump_stack(lua_State* L);
