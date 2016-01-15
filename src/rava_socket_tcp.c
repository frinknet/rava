#include <string.h>
#include "rava.h"
#include "rava_core.h"
#include "rava_socket.h"

int rava_new_tcp(lua_State* L)
{
  rava_state_t*  curr = ravaL_state_self(L);
  rava_object_t* self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));
  luaL_getmetatable(L, RAVA_SOCKET_TCP);
  lua_setmetatable(L, -2);

  ravaL_object_init(curr, self);

  uv_tcp_init(ravaL_event_loop(L), &self->h.tcp);

  return 1;
}

static int rava_tcp_bind(lua_State* L)
{
  rava_object_t *self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);

  struct sockaddr_in addr;
  const char* host;
  int port, r;

  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);

  uv_ip4_addr(host, port, &addr);

  r = uv_tcp_bind(&self->h.tcp, (const struct sockaddr*)&addr, 0);
  lua_pushinteger(L, r);

  return 1;
}

static int rava_tcp_connect(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);
  rava_state_t*  curr = ravaL_state_self(L);

  struct sockaddr_in addr;
  const char* host;
  int port;

  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);

  uv_ip4_addr(host, port, &addr);

  lua_settop(L, 2);

  int r = uv_tcp_connect(&curr->req.connect, &self->h.tcp,
		(const struct sockaddr*)&addr, ravaL_connect_cb);

  if (r < 0) {
    lua_settop(L, 0);
    lua_pushnil(L);
    lua_pushstring(L, uv_strerror(r));

    return 2;
  }

  return ravaL_state_suspend(curr);
}

static int rava_tcp_nodelay(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  int enable = lua_toboolean(L, 2);
  lua_settop(L, 2);
  int rv = uv_tcp_nodelay(&self->h.tcp, enable);
  lua_pushinteger(L, rv);
  return 1;
}

static int rava_tcp_keepalive(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  int enable = lua_toboolean(L, 2);
  unsigned int delay = 0;
  if (enable) {
    delay = luaL_checkint(L, 3);
  }
  int rv = uv_tcp_keepalive(&self->h.tcp, enable, delay);
  lua_settop(L, 1);
  lua_pushinteger(L, rv);
  return 1;
}

/* mostly stolen from Luvit */
static int rava_tcp_getsockname(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);

  int port = 0;
  char ip[INET6_ADDRSTRLEN];
  int family;

  struct sockaddr_storage addr;
  int len = sizeof(addr);
	int r = uv_tcp_getsockname(&self->h.tcp, (struct sockaddr*)&addr, &len);

  if (r < 0) {
    return luaL_error(L, "getsockname: %s", uv_strerror(r));
  }

  family = addr.ss_family;

  if (family == AF_INET) {
    struct sockaddr_in* addrin = (struct sockaddr_in*)&addr;
    uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin->sin_port);
	} else if (family == AF_INET6) {
    struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)&addr;
    uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin6->sin6_port);
  }

  lua_newtable(L);
  lua_pushnumber(L, port);
  lua_setfield(L, -2, "port");
  lua_pushnumber(L, family);
  lua_setfield(L, -2, "family");
  lua_pushstring(L, ip);
  lua_setfield(L, -2, "address");

  return 1;
}

/* mostly stolen from Luvit */
static int rava_tcp_getpeername(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);

  int port = 0;
  char ip[INET6_ADDRSTRLEN];
  int family;

  struct sockaddr_storage addr;
  int len = sizeof(addr);
	int r = uv_tcp_getpeername(&self->h.tcp, (struct sockaddr*)&addr, &len);

  if (r < 0) {
    return luaL_error(L, "getpeername: %s", uv_strerror(r));
  }

  family = addr.ss_family;
  if (family == AF_INET) {
    struct sockaddr_in* addrin = (struct sockaddr_in*)&addr;
    uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin->sin_port);
  }
  else if (family == AF_INET6) {
    struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)&addr;
    uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin6->sin6_port);
  }

  lua_newtable(L);
  lua_pushnumber(L, port);
  lua_setfield(L, -2, "port");
  lua_pushnumber(L, family);
  lua_setfield(L, -2, "family");
  lua_pushstring(L, ip);
  lua_setfield(L, -2, "address");

  return 1;
}

static int rava_tcp_tostring(lua_State *L)
{
  rava_object_t *self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_TCP);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_SOCKET_TCP, self);
  return 1;
}

luaL_Reg rava_tcp_meths[] = {
  {"bind",        rava_tcp_bind},
  {"connect",     rava_tcp_connect},
  {"getsockname", rava_tcp_getsockname},
  {"getpeername", rava_tcp_getpeername},
  {"keepalive",   rava_tcp_keepalive},
  {"nodelay",     rava_tcp_nodelay},
  {"__tostring",  rava_tcp_tostring},
  {NULL,          NULL}
};

LUA_API int loaopen_rava_socket_tcp(lua_State* L)
{
  ravaL_class(L, RAVA_SOCKET_TCP, rava_stream_meths);
  luaL_register(L, NULL, rava_tcp_meths);

  lua_pop(L, 1);
  luaL_pushcfunction(L, rava_new_tcp);

  return 1;
}
