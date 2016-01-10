
static int rava_lua_udp_new(lua_State *L)
{
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	uv_udp_t *handle = malloc(sizeof(uv_udp_t));
	register int r = uv_udp_init(g_loop, handle);
	if(r < 0) {
		free(handle);
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	handle->data = rava_lua_newuvobj(L, handle, "UDP");
	return 1;
}

static void rava_lua_on_send(uv_udp_send_t* req, int status)
{
	if(req->data) {
		luaZ_checkuvcb(req->data);
		if(status == 0) {
			lua_pushnil(L_Main);
		} else {
			lua_pushstring(L_Main, uv_err_name(status));
		}
		free(req);
		lua_call(L_Main, 1, 0);
	} else free(req);
}

static int rava_lua_udp_send(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	uv_buf_t buf;
	register int r;
	uv_udp_send_t *req;
	const char *ip;
	int port;
	struct sockaddr_storage addr;
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_UDP != obj->data->type)
		luaL_error(L, "expected a UDP handle, got %s", obj->tname);
	buf.base = (char*) luaL_checklstring(L, 2, &buf.len);
	ip = luaL_checkstring(L, 3);
	port = luaL_checkint(L, 4);
	if(uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
	   uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
		return luaL_error(L, "invalid IP address or port");
	req = malloc(sizeof(*req));
	if(lua_isfunction(L, 5)) {
		req->data = luaZ_newuvcb(L, 5, obj);
	} else {
		req->data = NULL;
	}
	r = uv_udp_send(req, (uv_udp_t *)obj->data, &buf, 1, (struct sockaddr *)&addr, rava_lua_on_send);
	if(r < 0) {
		if(req->data) luaZ_freeuvcb(req->data);
		free(req);
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static int rava_lua_udp_bind(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	const char *ip = luaL_checkstring(L, 2);
	int port = luaL_checkint(L, 3);
	register int r;
	struct sockaddr_storage addr;
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_UDP != obj->data->type)
		luaL_error(L, "expected a UDP handle, got %s", obj->tname);
	if(uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
	   uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
		return luaL_error(L, "invalid IP address or port");
	r = uv_udp_bind((uv_udp_t *)obj->data, (struct sockaddr *)&addr, 0);
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static void rava_lua_on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
	rava_lua_handle *obj = handle->data;
	if(nread == 0) { free(buf->base); return; }
	lua_rawgeti(L_Main, LUA_REGISTRYINDEX, obj->cbreadref);
	if (nread >= 0) {
		lua_pushlstring(L_Main, buf->base, nread);
		free(buf->base);
		rava_lua_pushaddr(L_Main, (struct sockaddr_storage*)addr, sizeof(*addr));
		lua_call(L_Main, 3, 0);
	} else {
		free(buf->base);
		lua_pushnil(L_Main);
		lua_pushstring(L_Main, uv_err_name(nread));
		lua_call(L_Main, 2, 0);
	}
}

static int rava_lua_udp_recv_start(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	register int r;

	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_UDP != obj->data->type)
		luaL_error(L, "expected a UDP handle, got %s", obj->tname);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	if(obj->cbreadref == LUA_REFNIL) {
		lua_pushvalue(L, 2);
		obj->cbreadref = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, 1);
		obj->cbreadself = luaL_ref(L, LUA_REGISTRYINDEX);
		r = uv_udp_recv_start((uv_udp_t *)obj->data, rava_lua_on_alloc, rava_lua_on_recv);
		if(r < 0) {
			luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadref);
			luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadself);
			obj->cbreadself = obj->cbreadref = LUA_REFNIL;
			lua_pushstring(L, uv_strerror(r));
			return lua_error(L);
		}
	} else {
		luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadref);
		lua_pushvalue(L, 2);
		obj->cbreadref = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	return 0;
}

static int rava_lua_udp_recv_stop(lua_State *L)
{
	rava_lua_handle *obj = luaL_checkudata(L, 1, RAVA_LUA_MT_HANDLE);
	register int r;
	
	RAVA_LUA_CHECK_CLOSED(obj);
	if(UV_UDP != obj->data->type)
		luaL_error(L, "expected a UDP handle, got %s", obj->tname);
	if(obj->cbreadref == LUA_REFNIL) return 0;
	luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadref);
	luaL_unref(L, LUA_REGISTRYINDEX, obj->cbreadself);
	obj->cbreadself = obj->cbreadref = LUA_REFNIL;
	r = uv_udp_recv_stop((uv_udp_t *)obj->data);
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

