int rava_new_cond(lua_State* L);
int ravaL_cond_init(rava_cond_t* cond);
int ravaL_cond_wait(rava_cond_t* cond, rava_state_t* curr);
int ravaL_cond_signal(rava_cond_t* cond);
int ravaL_cond_broadcast(rava_cond_t* cond);

LUA_API int luaopen_rava_process_cond(lua_State* L);
