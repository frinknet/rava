
static int rava_lua_fs_chdir(lua_State* L) {
	const char* dir = luaL_checkstring(L, 1);
	register int r = uv_chdir(dir);
	if(r < 0)  {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

