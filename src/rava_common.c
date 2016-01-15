#include "rava.h"

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

int ravaL_require(lua_State* L, const char* path)
{
  lua_getglobal(L, "require");
  lua_pushstring(L, path);
  lua_call(L, 1, 1);
  return 1;
}

int ravaL_lib_decoder(lua_State* L)
{
  const char* name = lua_tostring(L, -1);
  lua_getfield(L, LUA_REGISTRYINDEX, name);
  TRACE("LIB DECODE HOOK: %s\n", name);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    ravaL_require(L, name);
  }
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

int ravaL_extend(lua_State* L, const char* base, const char* name, luaL_Reg* meths)
{
  ravaL_class(L, name, meths);
  luaL_getmetatable(L, base);
  lua_setmetatable(L, -2);

  return 1;
}

void* ravaL_checkudata(lua_State* L, int idx, const char* name)
{
  luaL_checktype(L, idx, LUA_TUSERDATA);
  luaL_getmetatable(L, name);
  lua_pushvalue(L, idx);
  for (; lua_getmetatable(L, -1); ) {
    if (lua_equal(L, -1, -3)) {
      /* found it */
      lua_pop(L, 3);
      return lua_touserdata(L, idx);
    }
    else if (lua_equal(L, -1, -2)) {
      /* table is it's own metatable, end here */
      break;
    }
    else {
      /* up the inheritance chain */
      lua_replace(L, -2);
    }
  }
  luaL_error(L, "userdata<%s> expected at %i", name, idx);
  /* the above longjmp's anyway, but need to keep gcc happy */
  return NULL;
}

void ravaL_dump_stack(lua_State* L)
{
#ifdef RAVA_DEBUG
  int i;
  int top = lua_gettop(L);
  printf("--------------- STACK ---------------\n");
  for (i = top; i >= 1; i--) {
    int t = lua_type(L, i);
    printf("Stack[%2d - %8s] : ", i, lua_typename(L, t));
    switch (t) {
      case LUA_TSTRING:
        printf("%s", lua_tostring(L, i));
        break;
      case LUA_TBOOLEAN:
        printf(lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        printf("%g", lua_tonumber(L, i));
        break;
      case LUA_TNIL:
        printf("nil");
        break;
      case LUA_TFUNCTION:
        printf("function");
        break;
      case LUA_TTHREAD:
        printf("thread");
        break;
      case LUA_TUSERDATA:
        printf("userdata");
        break;
      default:
        printf("%s", lua_typename(L, t));
        break;
    }
    printf("\n");
  }
  printf("-------------------------------------\n");
#else
  (void)L;
#endif /* RAVA_DEBUG */
}
