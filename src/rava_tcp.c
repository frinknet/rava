#include "rava_tcp.h"
#include "rava_agent.h"
#include "rava_stream.h"

int ray_tcp_new(lua_State* L)
{
  ray_agent_t* self = ray_agent_new(L);
  luaL_getmetatable(L, "ray.tcp");
  lua_setmetatable(L, -2);
  uv_tcp_init(uv_default_loop(), &self->h.tcp);
  return 1;
}

int ray_tcp_accept(lua_State* L)
{
  lua_settop(L, 1);
  ray_tcp_new(L);

  return ray_accept(L);
}

int ray_tcp_connect(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  ray_fiber_t* curr = ray_current(L);

  struct sockaddr_in addr;
  const char* host;
  int port;

  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);
  uv_ip4_addr(host, port, &addr);

  int rc = uv_tcp_connect(&curr->r.connect, &self->h.tcp, (struct sockaddr *)&addr, ray_connect_cb);
  if (rc) return ray_push_error(L, rc);

  return ray_suspend(curr);
}

int ray_tcp_bind(lua_State* L)
{
  ray_agent_t *self = (ray_agent_t*)lua_touserdata(L, 1);

  struct sockaddr_in addr;
  const char* host;
  int port;

  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);
  uv_ip4_addr(host, port, &addr);

  int rc = uv_tcp_bind(&self->h.tcp, (struct sockaddr*)&addr, 0);
  if (rc) return ray_push_error(L, rc);

  lua_settop(L, 1);
  return 1;
}

int ray_tcp_nodelay(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.tcp");
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  int enable = lua_toboolean(L, 2);
  lua_settop(L, 2);

  int rc = uv_tcp_nodelay(&self->h.tcp, enable);
  if (rc) return ray_push_error(L, rc);

  lua_settop(L, 1);
  return 1;
}

int ray_tcp_keepalive(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.tcp");
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  int enable = lua_toboolean(L, 2);
  unsigned int delay = 0;
  if (enable) delay = luaL_checkint(L, 3);

  int rc = uv_tcp_keepalive(&self->h.tcp, enable, delay);
  if (rc) return ray_push_error(L, rc);

  lua_settop(L, 1);
  return 1;
}

int ray_tcp_getsockname(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.tcp");

  int port = 0;
  char ip[INET6_ADDRSTRLEN];
  int family;

  struct sockaddr_storage addr;
  int len = sizeof(addr);

  int rc = uv_tcp_getsockname(&self->h.tcp, (struct sockaddr*)&addr, &len);
  if (rc) return ray_push_error(L, rc);

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

int ray_tcp_getpeername(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.tcp");

  int port = 0;
  char ip[INET6_ADDRSTRLEN];
  int family;

  struct sockaddr_storage addr;
  int len = sizeof(addr);

  int rc = uv_tcp_getpeername(&self->h.tcp, (struct sockaddr*)&addr, &len);
  if (rc) return ray_push_error(L, rc);

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

static luaL_Reg ray_tcp_meths[] = {
  {"read",      ray_read},
  {"write",     ray_write},
  {"close",     ray_close},
  {"listen",    ray_listen},
  {"shutdown",  ray_shutdown},
  {"accept",    ray_tcp_accept},
  {"bind",      ray_tcp_bind},
  {"connect",   ray_tcp_connect},
  {"nodelay",   ray_tcp_nodelay},
  {"keepalive", ray_tcp_keepalive},
  {"getpeername", ray_tcp_getpeername},
  {"getsockname", ray_tcp_getsockname},
  {"__gc",      ray_agent_free},
  {NULL,        NULL}
};
