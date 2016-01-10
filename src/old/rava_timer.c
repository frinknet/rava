static int rava_lua_timer_new(lua_State *L)
{
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	uv_timer_t *handle = malloc(sizeof(uv_timer_t));
	register int r = uv_timer_init(g_loop, handle);
	if(r < 0) {
		free(handle);
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	handle->data = rava_lua_newuvobj(L, handle, "Timer");
	return 1;
}

static void rava_lua_on_timer(uv_timer_t* handle) {
	rava_lua_handle *obj = handle->data;
	lua_rawgeti(L_Main, LUA_REGISTRYINDEX, obj->cbref);
	lua_call(L_Main, 0, 0);
}

static int rava_lua_timer_start(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	register int r;
	int timeout, repeat;
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_TIMER != obj->data->type)
		luaL_error(L, "expected a Timer handle, got %s", obj->tname);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	timeout = luaL_checkint(L, 3);
	repeat = luaL_optint(L, 4, 0);
	if(obj->cbref != LUA_REFNIL) {
		luaL_unref(L, LUA_REGISTRYINDEX, obj->cbref);
		luaL_unref(L, LUA_REGISTRYINDEX, obj->cbself);
	}
	lua_pushvalue(L, 2);
	obj->cbref = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	obj->cbself = luaL_ref(L, LUA_REGISTRYINDEX);
	
	r = uv_timer_start((uv_timer_t *)obj->data, rava_lua_on_timer, timeout, repeat);
	if(r < 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, obj->cbref);
		luaL_unref(L, LUA_REGISTRYINDEX, obj->cbself);
		obj->cbself = obj->cbref = LUA_REFNIL;
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static int rava_lua_timer_stop(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	register int r;
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_TIMER != obj->data->type)
		luaL_error(L, "expected a Timer handle, got %s", obj->tname);
	if(obj->cbref == LUA_REFNIL) return 0;
	luaL_unref(L, LUA_REGISTRYINDEX, obj->cbref);
	luaL_unref(L, LUA_REGISTRYINDEX, obj->cbself);
	obj->cbself = obj->cbref = LUA_REFNIL;
	r = uv_timer_stop((uv_timer_t *)obj->data);
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static int rava_lua_timer_set_repeat(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_TIMER != obj->data->type)
		luaL_error(L, "expected a Timer handle, got %s", obj->tname);
	
	uv_timer_set_repeat((uv_timer_t *)obj->data, luaL_checkint(L, 2));
	return 0;
}

static int rava_lua_timer_get_repeat(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	register int r;
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_TIMER != obj->data->type)
		luaL_error(L, "expected a Timer handle, got %s", obj->tname);
	
	lua_pushinteger(L, uv_timer_get_repeat((uv_timer_t *)obj->data));
	return 1;
}


