static int rava_lua_system_uptime(lua_State* L) {
	double uptime;
	register int r = uv_uptime(&uptime);
	if(r < 0) {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	lua_pushnumber(L, uptime);
	return 1;
}

static int rava_lua_system_hrtime(lua_State* L) {
	lua_pushinteger(L, uv_hrtime());
	return 1;
}

static int rava_lua_system_chkupdate(lua_State* L) {
	uv_loop_t *g_loop = luaL_checkudata(L, lua_upvalueindex(1), RAVA_LUA_MT_LOOP);
	uv_update_time(g_loop);
	return 0;
}

static int rava_lua_system_cpus(lua_State* L) {
	uv_cpu_info_t* cpu_infos;
	int count, i;
	int ret = uv_cpu_info(&cpu_infos, &count);
	if (ret < 0) {
		lua_pushstring(L, uv_strerror(ret));
		return lua_error(L);
	}
	lua_newtable(L);
	for (i = 0; i < count; i++) {
		lua_newtable(L);
		lua_pushstring(L, cpu_infos[i].model);
		lua_setfield(L, -2, "model");
		lua_pushnumber(L, cpu_infos[i].speed);
		lua_setfield(L, -2, "speed");
		lua_newtable(L);
		lua_pushnumber(L, cpu_infos[i].cpu_times.user);
		lua_setfield(L, -2, "user");
		lua_pushnumber(L, cpu_infos[i].cpu_times.nice);
		lua_setfield(L, -2, "nice");
		lua_pushnumber(L, cpu_infos[i].cpu_times.sys);
		lua_setfield(L, -2, "sys");
		lua_pushnumber(L, cpu_infos[i].cpu_times.idle);
		lua_setfield(L, -2, "idle");
		lua_pushnumber(L, cpu_infos[i].cpu_times.irq);
		lua_setfield(L, -2, "irq");
		lua_setfield(L, -2, "times");
		lua_rawseti(L, -2, i + 1);
	}
	uv_free_cpu_info(cpu_infos, count);
	return 1;
}

static int rava_lua_system_memory(lua_State* L) {
	lua_pushnumber(L, uv_get_total_memory());
	return 1;
}

static int rava_lua_system_version(lua_State* L) {
	lua_pushstring(L, uv_version_string());
	return 1;
}

static int rava_lua_system_proctitle(lua_State* L) {
	const char* title = luaL_checkstring(L, 1);
	register int r = uv_set_process_title(title);
	if(r < 0)  {
		lua_pushstring(L, uv_strerror(r));
		return lua_error(L);
	}
	return 0;
}

