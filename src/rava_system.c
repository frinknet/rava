#include "rava_system.h"

void ray_system_getaddrinfo_cb(uv_getaddrinfo_t* req, int s, struct addrinfo* ai)
{
  ray_fiber_t* self = container_of(req, ray_fiber_t, r);
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

  lua_settop(self->L, 0);

  lua_pushstring (self->L, host);
  lua_pushinteger(self->L, port);

  uv_freeaddrinfo(ai);
  ray_resume(self, 2);
}

int ray_system_getaddrinfo(lua_State* L)
{
  ray_fiber_t* curr = ray_current(L);
  uv_getaddrinfo_t* req = &curr->r.getaddrinfo;

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
      else if (strcmp(s, "INET6") == 0) {
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
      if (strcmp(s, "STREAM") == 0) {
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
      if (strcmp(s, "TCP") == 0) {
        hints.ai_protocol = IPPROTO_TCP;
      }
      else if (strcmp(s, "UDP") == 0) {
        hints.ai_protocol = IPPROTO_UDP;
      }
      else {
        return luaL_error(L, "unsupported protocol: %s", s);
      }
    }
    lua_pop(L, 1);
  }

  uv_loop_t* loop = uv_default_loop();
  int rc = uv_getaddrinfo(loop, req, ray_system_getaddrinfo_cb, node, service, &hints);
  if (rc) return ray_push_error(L, rc);

  return ray_suspend(curr);
}

int ray_system_mem_free(lua_State* L)
{
  lua_pushinteger(L, uv_get_free_memory());
  return 1;
}

int ray_system_mem_total(lua_State* L)
{
  lua_pushinteger(L, uv_get_total_memory());
  return 1;
}

int ray_system_hrtime(lua_State* L)
{
  lua_pushinteger(L, uv_hrtime());
  return 1;
}

int ray_system_cpus(lua_State* L)
{
  int size, i;
  uv_cpu_info_t* info;
  int err = uv_cpu_info(&info, &size);

  lua_settop(L, 0);

  if (err) {
    lua_pushboolean(L, 0);
    luaL_error(L, uv_strerror(err));
    return 2;
  }

  lua_newtable(L);

  for (i = 0; i < size; i++) {
    lua_newtable(L);

    lua_pushstring(L, info[i].model);
    lua_setfield(L, -2, "model");

    lua_pushinteger(L, (lua_Integer)info[i].speed);
    lua_setfield(L, -2, "speed");

    lua_newtable(L); /* times */

    lua_pushinteger(L, (lua_Integer)info[i].cpu_times.user);
    lua_setfield(L, -2, "user");

    lua_pushinteger(L, (lua_Integer)info[i].cpu_times.nice);
    lua_setfield(L, -2, "nice");

    lua_pushinteger(L, (lua_Integer)info[i].cpu_times.sys);
    lua_setfield(L, -2, "sys");

    lua_pushinteger(L, (lua_Integer)info[i].cpu_times.idle);
    lua_setfield(L, -2, "idle");

    lua_pushinteger(L, (lua_Integer)info[i].cpu_times.irq);
    lua_setfield(L, -2, "irq");

    lua_setfield(L, -2, "times");

    lua_rawseti(L, 1, i + 1);
  }

  uv_free_cpu_info(info, size);
  return 1;
}

int ray_system_interfaces(lua_State* L)
{
  int size, i;
  char buf[INET6_ADDRSTRLEN];

  uv_interface_address_t* info;
  int err = uv_interface_addresses(&info, &size);

  lua_settop(L, 0);

  if (err) {
    lua_pushboolean(L, 0);
    luaL_error(L, uv_strerror(err));
    return 2;
  }

  lua_newtable(L);

  for (i = 0; i < size; i++) {
    uv_interface_address_t addr = info[i];

    lua_newtable(L);

    lua_pushstring(L, addr.name);
    lua_setfield(L, -2, "name");

    lua_pushboolean(L, addr.is_internal);
    lua_setfield(L, -2, "is_internal");

    if (addr.address.address4.sin_family == PF_INET) {
      uv_ip4_name(&addr.address.address4, buf, sizeof(buf));
    }
    else if (addr.address.address4.sin_family == PF_INET6) {
      uv_ip6_name(&addr.address.address6, buf, sizeof(buf));
    }

    lua_pushstring(L, buf);
    lua_setfield(L, -2, "address");

    lua_rawseti(L, -2, i + 1);
  }

  uv_free_interface_addresses(info, size);

  return 1;
}

static luaL_Reg ray_system_funcs[] = {
 {"cpus",       ray_system_cpus},
 {"mem_free",   ray_system_mem_free},
 {"mem_total",  ray_system_mem_total},
 {"hrtime",     ray_system_hrtime},
 {"interfaces", ray_system_interfaces},
 {"dnslookup",  ray_system_getaddrinfo},
};

LUA_API int LUA_MODULE(RAY_MODULE_SYSTEM, lua_State* L)
{
  rayL_module(L, RAY_MODULE_SYSTEM, system_funcs);
  lua_pop(L, 1);

  ray_init_main(L);

  return 1;
}
