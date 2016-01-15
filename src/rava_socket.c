#include "rava.h"
#include "rava_common.h"
#include <string.h>

LUA_API int luaopen_socket_pipe(lua_State* L);
LUA_API int luaopen_socket_udp(lua_State* L);
LUA_API int luaopen_socket_tcp(lua_State* L);

int rava_new_pipe(lua_State* L);
int rava_new_tcp(lua_State* L);
int rava_new_udp(lua_State* L);

static void _getaddrinfo_cb(uv_getaddrinfo_t* req, int s, struct addrinfo* ai)
{
  rava_state_t* curr = container_of(req, rava_state_t, req);
  char host[INET6_ADDRSTRLEN];
  int  port = 0;

  if (ai->ai_family == PF_INET) {
    struct sockaddr_in* addr = (struct sockaddr_in*)ai->ai_addr;
    uv_ip4_name(addr, host, INET6_ADDRSTRLEN);
    port = addr->sin_port;
  }
  else if (ai->ai_family == PF_INET6) {
    struct sockaddr_in6* addr = (struct sockaddr_in6*)ai->ai_addr;
    uv_ip6_name(addr, host, INET6_ADDRSTRLEN);
    port = addr->sin6_port;
  }
  lua_settop(curr->L, 0);
  lua_pushstring(curr->L, host);
  lua_pushinteger(curr->L, port);

  uv_freeaddrinfo(ai);

  ravaL_state_ready(curr);
}

static int rava_dns_resolve(lua_State* L)
{
  rava_state_t* curr = ravaL_state_self(L);
  uv_loop_t*   loop = ravaL_event_loop(L);
  uv_getaddrinfo_t* req = &curr->req.getaddrinfo;

  const char* node      = NULL;
  const char* service   = NULL;
  struct addrinfo hints;

  if (!lua_isnoneornil(L, 1)) {
    node = luaL_checkstring(L, 1);
  }
  if (!lua_isnoneornil(L, 2)) {
    service = luaL_checkstring(L, 2);
  }
  if (node == NULL && service == NULL) {
    return luaL_error(L, "getaddrinfo: provide either node or service");
  }

  hints.ai_family   = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags    = 0;

  if (lua_istable(L, 3)) {
    lua_getfield(L, 3, "family");
    if (!lua_isnil(L, -1)) {
      const char* s = lua_tostring(L, -1);
      if (strcmp(s, "INET") == 0) {
        hints.ai_family = PF_INET;
      }
      else if (strcmp(s, "INET6")) {
        hints.ai_family = PF_INET6;
      }
      else {
        return luaL_error(L, "unsupported family: %s", s);
      }
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "socktype");
    if (!lua_isnil(L, -1)) {
      const char* s = lua_tostring(L, -1);
      if (strcmp(s, "STREAM")) {
        hints.ai_socktype = SOCK_STREAM;
      }
      else if (strcmp(s, "DGRAM")) {
        hints.ai_socktype = SOCK_DGRAM;
      }
      else {
        return luaL_error(L, "unsupported socktype: %s", s);
      }
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "protocol");
    if (!lua_isnil(L, -1)) {
      const char* s = lua_tostring(L, -1);
      if (strcmp(s, "TCP")) {
        hints.ai_protocol = IPPROTO_TCP;
      }
      else if (strcmp(s, "UDP")) {
        hints.ai_protocol = IPPROTO_UDP;
      }
      else {
        return luaL_error(L, "unsupported protocol: %s", s);
      }
    }
    lua_pop(L, 1);
  }

  int r = uv_getaddrinfo(loop, req, _getaddrinfo_cb, node, service, &hints);

  if (r) {
    return luaL_error(L, uv_strerror(r));
  }

  return ravaL_state_suspend(curr);
}

luaL_Reg rava_socket_funcs[] = {
  {"pipe",        rava_new_pipe},
  {"tcp",         rava_new_tcp},
  {"udp",         rava_new_udp},
  {"dnsresolve",  rava_dns_resolve},
  {NULL,          NULL}
};

LUA_API int luaopen_rava_socket(lua_State* L)
{
	ravaL_module(L, RAVA_SOCKET, rava_socket_funcs);

	luaopen_rava_socket_pipe(L);
	luaopen_rava_socket_udp(L);
	luaopen_rava_socket_tcp(L);

	lua_pop(L, 1);

	return 1;
}
