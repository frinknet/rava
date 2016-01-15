#include "rava.h"

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

static int MAIN_INITIALIZED = 0;

int ravaL_traceback(lua_State* L)
{
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }

  lua_pushvalue(L, 1);    /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);      /* call debug.traceback */

  return 1;
}

int ravaL_lib_decoder(lua_State* L)
{
  const char* name = lua_tostring(L, -1);

  lua_getfield(L, LUA_REGISTRYINDEX, name);

  TRACE("LIB DECODE HOOK: %s\n", name);

  assert(lua_istable(L, -1));

  return 1;
}

/* return "rava:lib:decoder", <modname> */
int ravaL_lib_encoder(lua_State* L)
{
  TRACE("LIB ENCODE HOOK\n");

  lua_pushstring(L, "rava:lib:decoder");

  lua_getfield(L, 1, "__name");

  assert(!lua_isnil(L, -1));

  return 2;
}

uv_loop_t* ravaL_event_loop(lua_State* L)
{
  return ravaL_state_self(L)->loop;
}

int ravaL_new_module(lua_State* L, const char* name, luaL_Reg* funcs)
{
  lua_newtable(L);

  lua_pushstring(L, name);
  lua_setfield(L, -2, "__name");

  lua_pushvalue(L, -1);
  lua_setmetatable(L, -2);

  lua_pushcfunction(L, ravaL_lib_encoder);
  lua_setfield(L, -2, "__serialize");

  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_REGISTRYINDEX, name);

  if (funcs) {
    luaL_register(L, NULL, funcs);
  }
  return 1;
}

int ravaL_new_class(lua_State* L, const char* name, luaL_Reg* meths)
{
  luaL_newmetatable(L, name);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  if (meths) {
    luaL_register(L, NULL, meths);
  }
  return 1;
}
