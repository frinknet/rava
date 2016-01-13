#include "rava.h"

int ravaL_state_is_thread(rava_state_t* state) {
  return state->type == RAVA_TTHREAD;
}
int ravaL_state_in_thread(rava_state_t* state) {
  return state->type == RAVA_TTHREAD;
}

rava_state_t* ravaL_state_self(lua_State* L) {
  lua_pushthread(L);
  lua_rawget(L, LUA_REGISTRYINDEX);
  rava_state_t* self = (rava_state_t*)lua_touserdata(L, -1);
  lua_pop(L, 1);
  return self;
}

int ravaL_state_is_active(rava_state_t* state) {
  return state == ravaL_thread_self(state->L)->curr;
}

/* resume at the next iteration of the loop */
void ravaL_state_ready(rava_state_t* state) {
  if (state->type == RAVA_TTHREAD) {
    ravaL_thread_ready((rava_thread_t*)state);
  }
  else {
    ravaL_fiber_ready((rava_fiber_t*)state);
  }
}

/* yield a timeslice and allow passing values back to caller of resume */
int ravaL_state_yield(rava_state_t* state, int narg) {
  assert(ravaL_state_is_active(state));
  if (state->type == RAVA_TTHREAD) {
    return ravaL_thread_yield((rava_thread_t*)state, narg);
  }
  else {
    return ravaL_fiber_yield((rava_fiber_t*)state, narg);
  }
}

/* suspend execution of the current state until something wakes us up. */
int ravaL_state_suspend(rava_state_t* state) {
  if (state->type == RAVA_TTHREAD) {
    return ravaL_thread_suspend((rava_thread_t*)state);
  }
  else {
    return ravaL_fiber_suspend((rava_fiber_t*)state);
  }
}

/* transfer control directly to another state */
int ravaL_state_resume(rava_state_t* state, int narg) {
  if (state->type == RAVA_TTHREAD) {
    return ravaL_thread_resume((rava_thread_t*)state, narg);
  }
  else {
    return ravaL_fiber_resume((rava_fiber_t*)state, narg);
  }
}
