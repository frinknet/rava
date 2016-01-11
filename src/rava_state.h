#include "rava_lib.h"

ray_fiber_t* ray_current(lua_State* L);
int ray_suspend(ray_fiber_t* fiber);
void ray_sched_cb(uv_idle_t* idle);
int ray_ready(ray_fiber_t* fiber);
int ray_finish(ray_fiber_t* self);
int ray_resume(ray_fiber_t* fiber, int narg);
int ray_push_error(lua_State* L, uv_errno_t err);
int ray_run(lua_State* L);
int ray_self(lua_State* L);
