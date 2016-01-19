#define RAVA_FS_CALL(L, func, misc, ...) \
	do { \
    rava_state_t* curr = ravaL_state_self(L); \
    uv_loop_t*   loop = ravaL_event_loop(L); \
    uv_fs_t*     req; \
    uv_fs_cb     cb; \
    req = &curr->req.fs; \
		\
    if (curr->type == RAVA_STATE_TYPE_THREAD) { \
      /* synchronous in main */ \
      cb = NULL; \
		} else { \
      cb = rava_fs_cb; \
    } \
    req->data = misc; \
    \
		int r = uv_fs_##func(loop, req, __VA_ARGS__, cb); \
		\
    if (r < 0) { \
      lua_settop(L, 0); \
      lua_pushboolean(L, 0); \
      lua_pushstring(L, uv_strerror(r)); \
    } \
		\
    if (curr->type == RAVA_STATE_TYPE_THREAD) { \
      rava_fs_result(L, req); \
			\
      return lua_gettop(L); \
		} else { \
      TRACE("suspending...\n"); \
			\
      return ravaL_state_suspend(curr); \
    } \
  } while(0)

LUA_API int luaopen_rava_fs(lua_State* L);
