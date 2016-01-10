
static void rava_lua_pushaddr(lua_State* L, struct sockaddr_storage* address, int addrlen)
{
	char ip[INET6_ADDRSTRLEN];
	int port = 0;

	if (address->ss_family == AF_INET) {
		struct sockaddr_in* addrin = (struct sockaddr_in*)address;
		uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, addrlen);
		port = ntohs(addrin->sin_port);
	} else if (address->ss_family == AF_INET6) {
		struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)address;
		uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, addrlen);
		port = ntohs(addrin6->sin6_port);
	}

	lua_pushstring(L, ip);
	lua_pushinteger(L, port);
}

static rava_lua_stream *rava_lua_pushstream(lua_State *L, uv_handle_t *handle)
{
	rava_lua_stream *self = lua_newuserdata(L, sizeof(rava_lua_stream));
	self->handle = handle;
	self->flags = 0;
	self->cb_close = self->cb_data = LUA_REFNIL;
	luaL_getmetatable(L, RAVA_LUA_MT_STREAM);
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	self->ref_self = luaL_ref(L, LUA_REGISTRYINDEX);
	handle->data = self;
	return self;
}

static rava_lua_server *rava_lua_pushserver(lua_State *L, uv_handle_t *handle)
{
	rava_lua_server *self = lua_newuserdata(L, sizeof(rava_lua_server));
	self->handle = handle;
	luaL_getmetatable(L, RAVA_LUA_MT_SERVER);
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	self->ref_self = luaL_ref(L, LUA_REGISTRYINDEX);
	handle->data = self;
	self->ref_loop = self->cb_conn = LUA_REFNIL;
	return self;
}

static int l_stream__newindex(lua_State *L)
{
	// assert(type(k) == "string")
	luaL_checktype(L, 2, LUA_TSTRING);
	// local f = setters[k]
	lua_pushvalue(L, 2);
	lua_rawget(L, lua_upvalueindex(1));
	// assert(type(f) == "function")
	if(!lua_isfunction(L, -1))
		return luaL_error(L, "no such setter or setter bad type");
	// f(self, v or nil)
	lua_pushvalue(L, 1);
	if(lua_isnoneornil(L, 3)) {
		lua_pushnil(L);
	} else {
		lua_pushvalue(L, 3);
	}
	lua_call(L, 2, 0);
	return 0;
}

static int l_stream_set_on_data(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	if(NULL == self->handle) return 0;
	if(self->flags & RAVA_LUA_FLAG_READING) {
		if(lua_isnoneornil(L, 2))
			return luaL_error(L, "reading handles must have a callback");
	}
	if(self->cb_data != LUA_REFNIL)
		luaL_unref(L, LUA_REGISTRYINDEX, self->cb_data);
	if(lua_isnoneornil(L, 2))
		self->cb_data = LUA_REFNIL;
	else {
		luaL_checktype(L, 2, LUA_TFUNCTION);
		lua_pushvalue(L, 2);
		self->cb_data = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	return 0;
}

static int l_stream_set_on_close(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	if(self->handle) {
		if(self->cb_close != LUA_REFNIL)
			luaL_unref(L, LUA_REGISTRYINDEX, self->cb_close);
		if(lua_isnoneornil(L, 2))
			self->cb_close = LUA_REFNIL;
		else {
			luaL_checktype(L, 2, LUA_TFUNCTION);
			lua_pushvalue(L, 2);
			self->cb_close = luaL_ref(L, LUA_REGISTRYINDEX);
		}
	}
	return 0;
}

static void rava_lua_on_shutdown(uv_shutdown_t* req, int status)
{
	rava_lua_stream *self = req->data;
	lua_State *L = L_Main;
	uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
	free(req);
	RAVA_LUA_LUA_UNREF(self->ref_self);
	RAVA_LUA_LUA_UNREF(self->cb_data);
	if(self->cb_close != LUA_REFNIL) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, self->cb_close);
		luaL_unref(L, LUA_REGISTRYINDEX, self->cb_close);
		self->cb_close = LUA_REFNIL;
		lua_pushliteral(L, "over");
		lua_call(L, 1, 0);
	}
}

static int l_stream_close(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	if(self->handle) {
		int directly_closing = 1;
		if(self->flags & RAVA_LUA_FLAG_USABLE) {
			uv_shutdown_t *req = malloc(sizeof(uv_shutdown_t));
			req->data = self;
			if(!req) { uv_close(self->handle, (uv_close_cb)free); } else
			if(uv_shutdown(req, (uv_stream_t *)self->handle, rava_lua_on_shutdown) < 0) {
				free(req);
				uv_close(self->handle, (uv_close_cb)free);
			}
			directly_closing = 0; // defer removing callbacks
		} else uv_close(self->handle, (uv_close_cb)free);
		self->handle = NULL; self->flags = 0;
		if(directly_closing) {
			RAVA_LUA_LUA_UNREF(self->ref_self);
			RAVA_LUA_LUA_UNREF(self->cb_data);
			if(self->cb_close != LUA_REFNIL) {
				int has_no_arg = lua_isnoneornil(L, 2);
				lua_rawgeti(L, LUA_REGISTRYINDEX, self->cb_close);
				luaL_unref(L, LUA_REGISTRYINDEX, self->cb_close);
				self->cb_close = LUA_REFNIL;
				if(has_no_arg) {
					lua_call(L, 0, 0);
				} else {
					lua_pushvalue(L, 2);
					lua_call(L, 1, 0);
				}
			}
		}
	}
	return 0;
}

static int l_stream_alive(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	if(self->handle && (self->flags & RAVA_LUA_FLAG_USABLE)) {
		lua_pushvalue(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int l_stream__gc(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	if(self->handle) {
		uv_close(self->handle, (uv_close_cb)free);
		fprintf(stderr, "lua-rava: forgot to close stream 0x%X\n", (unsigned int)self->handle);
		self->handle = NULL;
	}
	return 0;
}

static void rava_lua_on_write(uv_write_t *req, int status)
{
	rava_lua_callback *cb = req->data;
	free(req);
	if(cb) {
		rava_lua_stream *self = cb->self;
		int cb_ref = cb->l_ref;
		free(cb);
		if(NULL == self->handle) return;
		lua_rawgeti(L_Main, LUA_REGISTRYINDEX, cb_ref);
		luaL_unref(L_Main, LUA_REGISTRYINDEX, cb_ref);
		if(status == 0) {
			lua_call(L_Main, 0, 0);
		} else {
			self->flags &= ~RAVA_LUA_FLAG_USABLE;
			RAVA_LUA_DEBUG("write failed!");
			// self:close(err)
			lua_pushcfunction(L_Main, l_stream_close);
			lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->ref_self);
			lua_pushstring(L_Main, uv_err_name(status));
			lua_call(L_Main, 2, 0);
			// callback(err)
			lua_pushstring(L_Main, uv_err_name(status));
			lua_call(L_Main, 1, 0);
		}
	}
}

static int l_stream_write(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	uv_buf_t buf;
	uv_write_t *req;
	rava_lua_callback *cb = NULL;
	int r;
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	if(!(self->flags & RAVA_LUA_FLAG_USABLE))
		return luaL_error(L, "stream is not open");
	// TODO: data to send must be a string currently, tables should be accepted
	buf.base = (char *) luaL_checklstring(L, 2, &buf.len);
	
	if(!lua_isnoneornil(L, 3)) {
		luaL_checktype(L, 3, LUA_TFUNCTION);
		lua_pushvalue(L, 3);
		cb = malloc(sizeof(rava_lua_callback));
		cb->l_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		cb->self = self;
	}
	req = malloc(sizeof(*req));
	req->data = cb;
	r = uv_write(req, (uv_stream_t *)self->handle, &buf, 1, rava_lua_on_write);
	if(r < 0) {
		if(cb) {
			luaL_unref(L, LUA_REGISTRYINDEX, cb->l_ref);
			free(cb);
		}
		free(req);
		lua_pushcfunction(L_Main, l_stream_close);
		lua_pushvalue(L, 1);
		lua_pushstring(L_Main, uv_err_name(r));
		lua_call(L_Main, 2, 0);
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	lua_pushvalue(L, 1);
	return 1;
}

static void rava_lua_on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = malloc(suggested_size);
	buf->len = buf->base ? suggested_size : 0;
}

static void rava_lua_on_data(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
	rava_lua_stream *self = handle->data;
	if (nread > 0) {
		lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->cb_data);
		lua_pushlstring(L_Main, buf->base, nread);
		free(buf->base);
		lua_call(L_Main, 1, 0);
	} else {
		if(buf->base) free(buf->base);
		self->flags &= ~RAVA_LUA_FLAG_USABLE;
		if (nread == UV_EOF) {
			lua_pushcfunction(L_Main, l_stream_close);
			lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->ref_self);
			lua_call(L_Main, 1, 0);
		} else if(nread < 0) {
			lua_pushcfunction(L_Main, l_stream_close);
			lua_rawgeti(L_Main, LUA_REGISTRYINDEX, self->ref_self);
			lua_pushstring(L_Main, uv_err_name(nread));
			lua_call(L_Main, 2, 0);
		}
	}
}

static int l_stream_read_start(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	int r;
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	if(!(self->flags & RAVA_LUA_FLAG_USABLE))
		return luaL_error(L, "stream is not open");
	if(LUA_REFNIL == self->cb_data)
		return luaL_error(L, "first of all set a callback");
	r = uv_read_start((uv_stream_t *)self->handle, rava_lua_on_alloc, rava_lua_on_data);
	if(r < 0) {
		lua_pushcfunction(L_Main, l_stream_close);
		lua_pushvalue(L, 1);
		lua_pushstring(L_Main, uv_err_name(r));
		lua_call(L_Main, 2, 0);
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	self->flags |= RAVA_LUA_FLAG_READING;
	lua_pushvalue(L, 1);
	return 1;
}

static int l_stream_read_stop(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	int r;
	if(!(self->flags & RAVA_LUA_FLAG_READING && self->flags & RAVA_LUA_FLAG_USABLE))
		return 0; // this stream is not being readed or being closed ...
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	r = uv_read_stop((uv_stream_t *)self->handle);
	if(r < 0) {
		lua_pushcfunction(L_Main, l_stream_close);
		lua_pushvalue(L, 1);
		lua_pushstring(L_Main, uv_err_name(r));
		lua_call(L_Main, 2, 0);
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	self->flags &= ~RAVA_LUA_FLAG_READING;
	lua_pushvalue(L, 1);
	return 1;
}

static int l_stream_getsockname(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	int addrlen, r;
	struct sockaddr_storage address;
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	if(UV_TCP != self->handle->type)
		return luaL_error(L, "stream is not a TCP stream");
	addrlen = sizeof(address);
	r = uv_tcp_getsockname(
		(uv_tcp_t *)self->handle,
		(struct sockaddr*)&address, &addrlen);
	if(r < 0) {
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	rava_lua_pushaddr(L, &address, addrlen);
	return 2;
}

static int l_stream_getpeername(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	int addrlen, r;
	struct sockaddr_storage address;
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	if(UV_TCP != self->handle->type)
		return luaL_error(L, "stream is not a TCP stream");
	addrlen = sizeof(address);
	r = uv_tcp_getpeername(
		(uv_tcp_t *)self->handle,
		(struct sockaddr*)&address, &addrlen);
	if(r < 0) {
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	rava_lua_pushaddr(L, &address, addrlen);
	return 2;
}

#ifdef __linux__

#include <linux/netfilter_ipv4.h>

/*
 * libuv doesn't provide this and this is used for my own purpose.
 * This is useful for transparent proxying...
 */

static int l_stream_originaldst(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	int addrlen, r;
	struct sockaddr_in6 address;
	uv_os_fd_t fd;
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	if(UV_TCP != self->handle->type)
		return luaL_error(L, "stream is not a TCP stream");
	r = uv_fileno(self->handle, &fd);
	if(r < 0) {
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	addrlen = sizeof(address);
	if (getsockopt(fd, SOL_IP, SO_ORIGINAL_DST,
		(struct sockaddr *)&address, &addrlen) != 0) {
		lua_pushnil(L);
		return 1;
	}
	rava_lua_pushaddr(L, (void*)&address, addrlen);
	return 2;
}

#endif

static int l_stream_nodelay(lua_State *L)
{
	rava_lua_stream *self = luaL_checkudata(L, 1, RAVA_LUA_MT_STREAM);
	int r;
	if(NULL == self->handle)
		return luaL_error(L, "attempt to operate on a closed stream");
	luaL_checktype(L, 2, LUA_TBOOLEAN);
	if(UV_TCP != self->handle->type)
		return luaL_error(L, "stream is not a TCP stream");
	r = uv_tcp_nodelay((uv_tcp_t *)self->handle, lua_toboolean(L, 2));
	if(r < 0) {
		lua_pushnil(L);
		lua_pushstring(L, uv_strerror(r));
		return 2;
	}
	lua_pushvalue(L, 1);
	return 1;
}

static luaL_Reg lreg_stream_newindex[] = {
	{ "on_data", l_stream_set_on_data },
	{ "on_close", l_stream_set_on_close },
	{ NULL, NULL }
};

static luaL_Reg lreg_stream_methods[] = {
	{ "getsockname", l_stream_getsockname },
	{ "getpeername", l_stream_getpeername },
#ifdef __linux__
	{ "originaldst", l_stream_originaldst },
#endif
	{ "nodelay", l_stream_nodelay },
	{ "read_start", l_stream_read_start },
	{ "read_stop", l_stream_read_stop },
	{ "close", l_stream_close },
	{ "write", l_stream_write },
	{ NULL, NULL }
};

