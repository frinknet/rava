#include "rava_lib.h"

void ray_system_getaddrinfo_cb(uv_getaddrinfo_t* req, int s, struct addrinfo* ai);
int ray_system_getaddrinfo(lua_State* L);
int ray_system_mem_free(lua_State* L);
int ray_system_mem_total(lua_State* L);
int ray_system_hrtime(lua_State* L);
int ray_system_cpus(lua_State* L);
int ray_system_interfaces(lua_State* L);
