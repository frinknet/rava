#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

int main(int argc, char *argv[]) {
	int i;

	lua_State* L = luaL_newstate();

	if (!L) {
		printf("Error: LuaJIT unavailable!!\n");
	}

	luaL_openlibs(L);
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
