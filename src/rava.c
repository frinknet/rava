#include "rava.h"

#include "rava_stream.c"
#include "rava_tcp.c"
#include "rava_handle.c"
#include "rava_udp.c"
#include "rava_timer.c"
#include "rava_system.c"
#include "rava_fs.c"

static int rava_lua_main_run(lua_State *L)
{
	int r;
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	if(L_Main) return luaL_error(L, "calling uv.run in a callback");
	L_Main = L;
	r = uv_run(g_loop, UV_RUN_DEFAULT);
	L_Main = NULL;
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static int rava_lua_main_run_nowait(lua_State *L)
{
	int r;
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	if(L_Main) return luaL_error(L, "calling uv.run_nowait in a callback");
	L_Main = L;
	r = uv_run(g_loop, UV_RUN_NOWAIT);
	L_Main = NULL;
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static int rava_lua_main_kill(lua_State* L)
{
	int pid = luaL_checkinteger(L, 1);
	int signum = luaL_optinteger(L, 2, SIGTERM);
	register int r = uv_kill(pid, signum);
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

static int rava_lua_loop__gc(lua_State *L)
{
	uv_loop_t *loop = luaL_checkudata(L, 1, RAVA_LUA_MT_LOOP);
	if(uv_loop_close(loop) < 0) {
		return luaL_error(L, "cannot close the uv_loop_t");
	}
	return 0;
}

static luaL_Reg lreg_main[] = {
	{ "main_run", rava_lua_main_run },
	{ "main_run_nowait", rava_lua_main_run_nowait },
	{ "main_close", rava_lua_handle__gc },
	{ "main_kill", rava_lua_main_kill },

	{ "udp_new", rava_lua_udp_new },
	{ "udp_bind", rava_lua_udp_bind },
	{ "udp_send", rava_lua_udp_send },
	{ "udp_recv_start", rava_lua_udp_recv_start },
	{ "udp_recv_stop", rava_lua_udp_recv_stop },

	{ "tcp_connect", rava_lua_tcp_connect },
	{ "tcp_listen", rava_lua_tcp_listen },
	{ "pipe_connect", rava_lua_pipe_connect },

	{ "timer_new", rava_lua_timer_new },
	{ "timer_start", rava_lua_timer_start },
	{ "timer_stop", rava_lua_timer_stop },
	{ "timer_get_repeat", rava_lua_timer_get_repeat },
	{ "timer_set_repeat", rava_lua_timer_set_repeat },

	{ "fs_chdir", rava_lua_fs_chdir },

	{ "system_uptime", rava_lua_system_uptime },
	{ "system_hrtime", rava_lua_system_hrtime },
	{ "system_chkupdate", rava_lua_system_chkupdate },
	{ "system_cpus", rava_lua_system_cpus },
	{ "system_version", rava_lua_system_version },
	{ "system_memory", rava_lua_system_memory },
	{ "system_proctitle", rava_lua_system_proctitle },
	{ NULL, NULL }
};

LUA_API int luaopen_rava(lua_State *L)
{
	uv_loop_t *g_loop = lua_newuserdata(L, sizeof(uv_loop_t));
	{
		// Initialize the libUV loop
		int r = uv_loop_init(g_loop);

		// Make sure we got started
		if(r < 0) {
			lua_pushstring(L, uv_strerror(r));

			return lua_error(L);
		}

		// Rava:Loop metatable
		luaL_newmetatable(L, RAVA_LUA_MT_LOOP);
		lua_pushcfunction(L, rava_lua_loop__gc);
		lua_setfield(L, -2, "__gc");
		lua_setmetatable(L, -2);
	}

	// Rava:Stream metatable
	luaL_newmetatable(L, RAVA_LUA_MT_STREAM);
	lua_newtable(L);
	luaL_register(L, NULL, lreg_stream_newindex);
	lua_pushcclosure(L, l_stream__newindex, 1);
	lua_setfield(L, -2, "__newindex");
	lua_newtable(L);
	luaL_register(L, NULL, lreg_stream_methods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_stream__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, l_stream_alive);
	lua_setfield(L, -2, "__call");

	// Rava:Server metatable
	luaL_newmetatable(L, RAVA_LUA_MT_SERVER);
	lua_newtable(L);
	lua_pushcfunction(L, rava_lua_tcp_server_close);
	lua_setfield(L, -2, "close");
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, rava_lua_tcp_server_close);
	lua_setfield(L, -2, "__gc");

	// Rava:Handler metatable
	luaL_newmetatable(L, RAVA_LUA_MT_HANDLE);
	luaL_register(L, NULL, lreg_handle);
	
	lua_pop(L, 3);
	lua_newtable(L);

	{ // register the library functions
		int i = 0;

		while(lreg_main[i].name && lreg_main[i].func) {
			lua_pushvalue(L, -2);
			lua_pushcclosure(L, lreg_main[i].func, 1);
			lua_setfield(L, -2, lreg_main[i].name);

			i++;
		}
	}

	return 1;
}
