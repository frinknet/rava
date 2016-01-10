#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <uv.h>

static lua_State *L_Main = NULL;

typedef struct {
	uv_handle_t *data;
	const char *tname;
	int cbreadref, cbref, cbreadself, cbself;
} rava_lua_handle;

typedef struct {
	uv_handle_t *handle;
	int cb_close, cb_data, ref_self;
	int flags;
} rava_lua_stream;

typedef struct {
	uv_handle_t *handle;
	int cb_conn, ref_self, ref_loop;
} rava_lua_server;

typedef struct {
	uv_poll_t *handle;
	int cb_tick, ref_self;
} rava_lua_poll;

typedef struct {
	int l_ref, data_ref;
	rava_lua_stream *self;
} rava_lua_callback;

#define RAVA_LUA_MT_LOOP   "Rava:Loop"
#define RAVA_LUA_MT_STREAM "Rava:Stream"
#define RAVA_LUA_MT_SERVER "Rava:Server"
#define RAVA_LUA_MT_POLL "Rava:Poll"
#define RAVA_LUA_MT_HANDLE "Rava:Handle"
#define RAVA_LUA_FLAG_USABLE  0x1
#define RAVA_LUA_FLAG_READING 0x2
#define RAVA_LUA_LUA_UNREF(v) if(v != LUA_REFNIL) { \
	luaL_unref(L, LUA_REGISTRYINDEX, v); \
	v = LUA_REFNIL; \
}
#define RAVA_LUA_DEBUG(s) puts("lua-rava: [DEBUG] " s)

LUA_API int luaopen_rava(lua_State *L);
