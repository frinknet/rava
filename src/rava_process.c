#include "rava.h"
#include "rava_core.h"
#include "rava_process.h"

static void _async_cb(uv_async_t* handle)
{
  TRACE("interrupt loop\n");

  (void)handle;
}

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
	const char* set = luaL_checkstring(L, 1);
	char* title = (char*)set;
	size_t size;
	register int r;

	if (title) {
		r	= uv_set_process_title(set);
	} else {
		r	= uv_get_process_title(title, size);
	}

	if (r < 0)  {
		return luaL_error(L, uv_strerror(r));
	}

	lua_pushstring(L, title);

	return 1;
}

static void rava_process_init(lua_State* L)
{
  rava_thread_t* self = (rava_thread_t*)lua_newuserdata(L, sizeof(rava_thread_t));

  luaL_getmetatable(L, RAVA_PROCESS_THREAD);
  lua_setmetatable(L, -2);

  self->type  = RAVA_STATE_TYPE_THREAD;
  self->flags = RAVA_STATE_READY;
  self->loop  = uv_default_loop();
  self->curr  = (rava_state_t*)self;
  self->L     = L;
  self->outer = (rava_state_t*)self;
  self->data  = NULL;
  self->tid   = (uv_thread_t)uv_thread_self();

  QUEUE_INIT(&self->rouse);

  uv_async_init(self->loop, &self->async, _async_cb);
  uv_unref((uv_handle_t*)&self->async);

  lua_pushthread(L);
  lua_pushvalue(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);
}

luaL_Reg rava_process_funcs[] = {
  {"self",   rava_process_self},
  {"sleep",  rava_process_sleep},
  {"title",  rava_process_title},
  {NULL,     NULL}
};

LUA_API int luaopen_rava_process(lua_State* L)
{
	ravaL_module(L, RAVA_PROCESS, rava_process_funcs);

	luaopen_rava_process_cond(L);
	lua_setfield(L, -2, "cond");

	luaopen_rava_process_fiber(L);
	lua_setfield(L, -2, "fiber");

	luaopen_rava_process_idle(L);
	lua_setfield(L, -2, "idle");

	luaopen_rava_process_spawn(L);
	lua_setfield(L, -2, "spawn");

	luaopen_rava_process_thread(L);
	lua_setfield(L, -2, "thread");

	luaopen_rava_process_timer(L);
	lua_setfield(L, -2, "timer");

	rava_process_init(L);
	lua_pop(L, 1);

	/* proc.std{in,out,err} */
	uv_loop_t* loop = ravaL_event_loop(L);
	rava_state_t* curr = ravaL_state_self(L);

	const char* stdfhs[] = { "stdin", "stdout", "stderr" };
	int i;

	for (i = 0; i < 3; i++) {
#ifdef WIN32
		const uv_file fh = GetStdHandle(i == 0 ? STD_INPUT_HANDLE
			: (i == 1 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE));
#else
		const uv_file fh = i;
#endif
		rava_object_t* stdfh = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));

		luaL_getmetatable(L, RAVA_SOCKET_PIPE);
		lua_setmetatable(L, -2);

		ravaL_object_init(curr, stdfh);

		uv_pipe_init(loop, &stdfh->h.pipe, 0);
		uv_pipe_open(&stdfh->h.pipe, fh);

		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, (const char*)stdfhs[i]);
		lua_setfield(L, -2, (const char*)stdfhs[i]);
	}

  return 1;
}
