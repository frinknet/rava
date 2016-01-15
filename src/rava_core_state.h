int ravaL_state_is_thread(rava_state_t* state);
int ravaL_state_in_thread(rava_state_t* state);
rava_state_t* ravaL_state_self(lua_State* L);
int ravaL_state_is_active(rava_state_t* state);
int ravaL_state_xcopy(rava_state_t* a, rava_state_t* b);
void ravaL_state_ready(rava_state_t* state);
int ravaL_state_yield(rava_state_t* state, int narg);
int ravaL_state_suspend(rava_state_t* state);
int ravaL_state_resume(rava_state_t* state, int narg);
