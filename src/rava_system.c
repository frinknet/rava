#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef __cplusplus
}
#endif


#include "rava.h"

static int rava_system_cpus(lua_State* L)
{
  int size, i;
  uv_cpu_info_t* info;
  int r = uv_cpu_info(&info, &size);

  lua_settop(L, 0);

  if (r < 0) {
    lua_pushboolean(L, 0);
    luaL_error(L, uv_strerror(r));

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

static int rava_system_load(lua_State* L) {
	double loadavg;
	register int r = uv_loadavg(&loadavg);

	if(r < 0) {
		return luaL_error(L, uv_strerror(r));
	}

	lua_pushnumber(L, loadavg);

	return 1;
}

static int rava_system_memory(lua_State* L)
{
  lua_settop(L, 0);
  lua_newtable(L);
  lua_pushinteger(L, uv_get_free_memory());
  lua_setfield(L, -2, "free");
  lua_pushinteger(L, uv_get_total_memory());
  lua_setfield(L, -2, "total");

  return 1;
}

static int rava_system_interfaces(lua_State* L)
{
  int size, i;
  char buf[INET6_ADDRSTRLEN];

  uv_interface_address_t* info;
  int r = uv_interface_addresses(&info, &size);

  lua_settop(L, 0);

  if (r < 0) {
    lua_pushboolean(L, 0);
    luaL_error(L, uv_strerror(r));

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

static int rava_system_timecode(lua_State* L)
{
  lua_pushinteger(L, uv_hrtime());
  return 1;
}

static int rava_system_uptime(lua_State* L) {
	double uptime;
	register int r = uv_uptime(&uptime);

	if(r < 0) {
		return luaL_error(L, uv_strerror(r));
	}

	lua_pushnumber(L, uptime);

	return 1;
}

static int rava_system_serialize(lua_State* L)
{
  return ravaL_serialize_encode(L, lua_gettop(L));
}

static int rava_system_unserialize(lua_State* L)
{
  return ravaL_serialize_decode(L);
}

luaL_Reg rava_system_funcs[] = {
  {"fs",          rava_system_fs},
  {"cpus",        rava_system_cpus},
  {"load",        rava_system_load},
  {"memory",      rava_system_memory},
  {"interfaces",  rava_system_interfaces},
  {"timecode",    rava_system_timecode},
  {"uptime",      rava_system_uptime},
  {"serialize",   rava_system_serialize},
  {"unserialize", rava_system_unserialize},
  {NULL,          NULL}
};
