typedef struct {
	int cbref, cbself;
	rava_lua_handle *owner;
} luaZ_UVCallback;

static rava_lua_handle* rava_lua_newuvobj(lua_State *L, void *handle, const char *tn)
{
	rava_lua_handle *obj = lua_newuserdata(L, sizeof(rava_lua_handle));
	register int i;
	obj->data = handle;
	obj->tname = tn;
	obj->cbref = obj->cbreadref = LUA_REFNIL;
	obj->cbself = obj->cbreadself = LUA_REFNIL;
	luaL_getmetatable(L, RAVA_LUA_MT_HANDLE);
	lua_setmetatable(L, -2);
	return obj;
}

static luaZ_UVCallback* luaZ_newuvcb(lua_State *L, int idx, rava_lua_handle *obj)
{
	luaZ_UVCallback *cb = malloc(sizeof(luaZ_UVCallback));
	lua_pushvalue(L, idx);
	cb->cbref = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	cb->cbself = luaL_ref(L, LUA_REGISTRYINDEX);
	cb->owner = obj;
	return cb;
}

static void luaZ_freeuvcb(luaZ_UVCallback *cb)
{
	luaL_unref(L_Main, LUA_REGISTRYINDEX, cb->cbref);
	luaL_unref(L_Main, LUA_REGISTRYINDEX, cb->cbself);
	free(cb);
}

static void luaZ_checkuvcb(luaZ_UVCallback *cb)
{
	lua_rawgeti(L_Main, LUA_REGISTRYINDEX, cb->cbref);
	luaZ_freeuvcb(cb);
}

static int rava_lua_handle__gc(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	if(obj->data) {
		uv_close(obj->data, (uv_close_cb)free);
		if(obj->cbreadref != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadref);
			luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadself);
		}
		if(obj->cbref != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, obj->cbref);
			luaL_unref(L, LUA_REGISTRYINDEX, obj->cbself);
		}
		obj->cbref = obj->cbreadref = LUA_REFNIL;
		obj->cbself = obj->cbreadself = LUA_REFNIL;
		obj->data = NULL;
	}
	return 0;
}

static int rava_lua_handle__len(lua_State *L) // This returns if the handle is closed
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	lua_pushboolean(L, obj->data ? 1 : 0);
	return 1;
}

static int rava_lua_handle__tostring(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	lua_pushstring(L, obj->tname);
	return 1;
}

static luaL_Reg lreg_handle[] = {
	{ "__len", rava_lua_handle__len },
	{ "__gc", rava_lua_handle__gc },
	{ "__tostring", rava_lua_handle__tostring },
	{ NULL, NULL }
};
