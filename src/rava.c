#include <stdio.h>
#include <pthread.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "xuv.h"

/**
 * Create a lua container from and run the lua code you pass to it
 *
 * returns new lua state
 */
lua_State* rava_newlua() {
	int i;

	lua_State* L = luaL_newstate();

	if (!L) {
		printf("Error: LuaJIT unavailable!!\n");
	}

	luaL_openlibs(L);

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_remove(L, -2); // Remove package
	
	// Store uv module definition at preload.uv
	lua_pushcfunction(L, luaopen_xuv);
	lua_setfield(L, -2, "xuv");

	return L;
}

/**
 * Main loop sets up the first lua contaner with arg
 *
 * argc count of arguments
 * argv array of argument values
 *
 * returns return code
 */
int main(int argc, char *argv[]) {
	int i;

	lua_State* L = rava_newlua();

	lua_newtable(L);

	for (i = 0; i < argc; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, argv[i]);
		lua_settable(L, -3);
	}

	lua_setglobal(L, "arg");

	int r = luaL_dostring(L, "require \"init\"");

	if (r != 0) {
		printf("%s\n", lua_tolstring(L, -1, 0));

		return r;
	}

	return 0;
}
