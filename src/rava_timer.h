#include "rava_lib.h"

int ray_timer_new(lua_State* L);
void ray_timer_cb(uv_timer_t* timer);
int ray_timer_start(lua_State* L);
int ray_timer_stop(lua_State* L);
int ray_timer_wait(lua_State* L);
