#include "rava.h"

static void _sleep_cb(uv_timer_t* handle)
{
  ravaL_state_ready((rava_state_t*)handle->data);
  free(handle);
}

static int rava_process_sleep(lua_State* L)
{
  lua_Number timeout = luaL_checknumber(L, 1);
  rava_state_t* state = ravaL_state_self(L);
  uv_timer_t*  timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
  timer->data = state;

  uv_timer_init(ravaL_event_loop(L), timer);
  uv_timer_start(timer, _sleep_cb, (long)(timeout * 1000), 0L);

  return ravaL_state_suspend(state);
}

static int rava_process_self(lua_State* L)
{
  lua_pushthread(L);
  lua_gettable(L, LUA_REGISTRYINDEX);

  return 1;
}

static int rava_process_title(lua_State* L)
{
	const char* title = luaL_checkstring(L, 1);
	register int r;

	if (title) {
		r	= uv_set_process_title(title);
	} else {
		r	= uv_get_process_title(title);
	}

	if(r < 0)  {
		return luaL_error(L, uv_strerror(r));
	}

	lua_pushstring(L, title);

	return 1;
}

luaL_Reg rava_process_funcs[] = {
  {"self",        rava_process_self},
  {"sleep",       rava_process_sleep},
  {"spawn",       rava_new_process},
  {"idle",        rava_new_idle},
  {"fiber",       rava_new_fiber},
  {"thread",      rava_new_thread},
  {"conduit",     rava_new_cond},
  {"timer",       rava_new_timer},
  {NULL,          NULL}
};

