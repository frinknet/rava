#include "rava.h"
#include "rava_core.h"
#include "rava_core_state.h"
#include "rava_process_thread.h"
#include <stdio.h>

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
	rava_state_t* self = ravaL_state_self(L);
	uv_loop_t* loop = self->loop;

	return loop;
}

int ravaL_require(lua_State* L, const char* path)
{
	lua_getglobal(L, "require");
	lua_pushstring(L, path);
	lua_call(L, 1, 1);

	return 1;
}

int ravaL_module(lua_State* L, const char* name, luaL_Reg* funcs)
{
  TRACE("new module: %s, funcs: %p\n", name, funcs);
  lua_newtable(L);

  lua_pushstring(L, name);
  lua_setfield(L, -2, "__name");

  /* its own metatable */
  lua_pushvalue(L, -1);
  lua_setmetatable(L, -2);

  /* so that it can pass through a thread boundary */
  lua_pushcfunction(L, ravaL_lib_encoder);
  lua_setfield(L, -2, "__serialize");

  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_REGISTRYINDEX, name);

  if (funcs) {
    TRACE("register funcs...\n");
    luaL_register(L, NULL, funcs);
  }

  TRACE("DONE\n");
  return 1;
}

int ravaL_class(lua_State* L, const char* name, luaL_Reg* meths)
{
  TRACE("new class: %s, meths: %p\n", name, meths);
  luaL_newmetatable(L, name);

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");

  if (meths) luaL_register(L, NULL, meths);

  lua_pushstring(L, name);
  lua_setfield(L, -2, "__name");

  return 1;
}
