int rava_new_thread(lua_State* L);

void ravaL_thread_ready(rava_thread_t* self);
int ravaL_thread_yield(rava_thread_t* self, int narg);
int ravaL_thread_suspend(rava_thread_t* self);
int ravaL_thread_resume(rava_thread_t* self, int narg);
void ravaL_thread_enqueue(rava_thread_t* self, rava_fiber_t* fiber);
rava_thread_t* ravaL_thread_self(lua_State* L);
int ravaL_thread_once(rava_thread_t* self);
int ravaL_thread_loop(rava_thread_t* self);
rava_thread_t* ravaL_thread_create(rava_state_t* outer, int narg);

LUA_API int luaopen_rava_process_thread(lua_State* L);
