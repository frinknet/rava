int rava_new_fiber(lua_State* L);
void ravaL_fiber_close(rava_fiber_t* fiber);
void ravaL_fiber_ready(rava_fiber_t* fiber);
int ravaL_fiber_yield(rava_fiber_t* self, int narg);
int ravaL_fiber_suspend(rava_fiber_t* self);
int ravaL_fiber_resume(rava_fiber_t* self, int narg);
rava_fiber_t* ravaL_fiber_create(rava_state_t* outer, int narg);

LUA_API int luaopen_rava_process_fiber(lua_State* L);
