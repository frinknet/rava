
static void rava_lua_tcp_on_connect(uv_connect_t* req, int status)
{
	rava_lua_callback *cb = (rava_lua_callback *)req->data;
	rava_lua_stream *self = cb->self;
	int cb_ref = cb->l_ref;
	
	free(cb);
	free(req);
	if(NULL == self->handle) return;
	lua_rawgeti(L_Main, LUA_REGISTRYINDEX, cb_ref);
	luaL_unref(L_Main, LUA_REGISTRYINDEX, cb_ref);
	if(status == 0) {
		self->flags |= RAVA_LUA_FLAG_USABLE;
		lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->ref_self);
		lua_call(L_Main, 1, 0);
	} else {
		lua_pushcfunction(L_Main, l_stream_close);
		lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->ref_self);
		lua_call(L_Main, 1, 0);
		lua_pushnil(L_Main);
		lua_pushstring(L_Main, uv_err_name(status));
		lua_call(L_Main, 2, 0);
	}
}

static int rava_lua_tcp_connect(lua_State *L)
{
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	const char *ip = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	struct sockaddr_in addr;
	uv_tcp_t *stream;
	int r;

	if(uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
	   uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
		return luaL_error(L, "invalid IP address or port");
	luaL_checktype(L, 3, LUA_TFUNCTION);
	if(stream = malloc(sizeof(*stream))) {
		r = uv_tcp_init(g_loop, stream);
		if(r < 0) {
			free(stream);
			lua_pushstring(L, uv_strerror(r));
			return lua_error(L);
		}
	} else return luaL_error(L, "can't allocate memory");
	{
		rava_lua_stream *self = rava_lua_pushstream(L, (uv_handle_t *)stream);
		uv_connect_t *req = malloc(sizeof(uv_connect_t));
		rava_lua_callback *cb = malloc(sizeof(rava_lua_callback));
		lua_pushvalue(L, 3);
		cb->l_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		cb->self = self;
		req->data = cb;
		r = uv_tcp_connect(req, stream, (struct sockaddr *)&addr, rava_lua_tcp_on_connect);
		if(r < 0) {
			free(req);
			luaL_unref(L, LUA_REGISTRYINDEX, cb->l_ref);
			free(cb);
			lua_pushcfunction(L, l_stream_close);
			lua_pushvalue(L, -2);
			lua_call(L, 1, 0);
			lua_pushstring(L, uv_strerror(r));
			return lua_error(L);
		}
		return 1;
	}
}

static int rava_lua_pipe_connect(lua_State *L)
{
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	const char *name = luaL_checkstring(L, 1);
	int ipc = lua_toboolean(L, 2);
	uv_pipe_t *stream;
	int r;

	luaL_checktype(L, 3, LUA_TFUNCTION);
	if(stream = malloc(sizeof(*stream))) {
		r = uv_pipe_init(g_loop, stream, ipc);
		if(r < 0) {
			free(stream);
			lua_pushstring(L, uv_strerror(r));
			return lua_error(L);
		}
	} else return luaL_error(L, "can't allocate memory");
	{
		rava_lua_stream *self = rava_lua_pushstream(L, (uv_handle_t *)stream);
		uv_connect_t *req = malloc(sizeof(uv_connect_t));
		rava_lua_callback *cb = malloc(sizeof(rava_lua_callback));
		lua_pushvalue(L, 3);
		cb->l_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		cb->self = self;
		req->data = cb;
		uv_pipe_connect(req, stream, name, rava_lua_tcp_on_connect);
		return 1;
	}
}

static int rava_lua_tcp_server_close(lua_State *L)
{
	rava_lua_server *self = luaL_checkudata(L, 1, RAVA_LUA_MT_SERVER);
	if(self->handle) {
		uv_close(self->handle, (uv_close_cb)free);
		self->handle = NULL;
		RAVA_LUA_LUA_UNREF(self->ref_self);
		RAVA_LUA_LUA_UNREF(self->cb_conn);
		RAVA_LUA_LUA_UNREF(self->ref_loop);
	}
	return 0;
}

static void rava_lua_tcp_on_listen(uv_stream_t* handle, int status) {
	rava_lua_server *self = handle->data;
	if(status < 0) {
		lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->cb_conn);
		lua_pushnil(L_Main);
		lua_pushstring(L_Main, uv_err_name(status));
		lua_call(L_Main, 2, 0);
	} else {
		uv_loop_t *g_loop;
		uv_tcp_t *stream = malloc(sizeof(uv_tcp_t));
		int r;
		if(!stream) return;
		lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->ref_loop);
		g_loop = lua_touserdata(L_Main, -1);
		lua_pop(L_Main, 1);
		r = uv_tcp_init(g_loop, stream);
		if(r < 0) {
			free(stream);
			return;
		}
		r = uv_accept(handle, (uv_stream_t *)stream);
		if(r < 0) {
			uv_close((uv_handle_t *)stream, (uv_close_cb)free);
		} else {
			rava_lua_stream *client;
			lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->cb_conn);
			client = rava_lua_pushstream(L_Main, (uv_handle_t *)stream);
			client->flags |= RAVA_LUA_FLAG_USABLE;
			lua_call(L_Main, 1, 0);
		}
	}
}

static int rava_lua_tcp_listen(lua_State *L)
{
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	const char *ip = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	int backlog = luaL_checkinteger(L, 3);
	struct sockaddr_in addr;
	uv_tcp_t *stream;
	int r;

	if(uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
	   uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
		return luaL_error(L, "invalid IP address or port");
	luaL_checktype(L, 4, LUA_TFUNCTION);
	if(stream = malloc(sizeof(*stream))) {
		r = uv_tcp_init(g_loop, stream);
		if(r < 0) {
			free(stream);
			lua_pushstring(L, uv_strerror(r));
			return lua_error(L);
		}
	} else return luaL_error(L, "can't allocate memory");
	r = uv_tcp_bind((uv_tcp_t *)stream, (struct sockaddr *)&addr, 0);
	if(r < 0) { 
		uv_close((uv_handle_t *)stream, (uv_close_cb)free);
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);	
	}
	{
		rava_lua_server *self = rava_lua_pushserver(L, (uv_handle_t *)stream);
		lua_pushvalue(L, 4);
		self->cb_conn = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, lua_upvalueindex(1));
		self->ref_loop = luaL_ref(L, LUA_REGISTRYINDEX);
		r = uv_listen((uv_stream_t *)stream, backlog, rava_lua_tcp_on_listen);
		if(r < 0) { 
			lua_pushcfunction(L, rava_lua_tcp_server_close);
			lua_pushvalue(L, -2);
			lua_call(L, 1, 0);
			lua_pushstring(L, uv_strerror(r));
			return lua_error(L);	
		}
		return 1;
	}
}
