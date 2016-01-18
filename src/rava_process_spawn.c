#include "rava.h"
#include "rava_core.h"
#include "rava_process.h"

/*
  rava.process.spawn("cat", {
    "foo.txt",
    env    = { HOME = "/home/cnorris" },
    cwd    = "/tmp",
    stdin  = rava.stdout,
    stdout = rava.stdin,
    stderr = rava.stderr,
    detach = true,
  })
*/

static void _exit_cb(uv_process_t* handle, int64_t status, int sigterm)
{
  TRACE("EXIT : status %i, sigterm %i\n", status, sigterm);
  rava_object_t* self = container_of(handle, rava_object_t, h);

  if (status < 0) {
    TRACE("ERROR: %s\n", uv_strerror(status));
  }

  lua_State* L = self->state->L;
  lua_pushinteger(L, status);
  lua_pushinteger(L, sigterm);

  ravaL_cond_signal(&self->rouse);
}

int rava_new_spawn(lua_State* L)
{
  const char* cmd = luaL_checkstring(L, 1);
  size_t argc;
  char** args;
  int i;
  char* cwd;
  char** env;
  uv_process_options_t* opts;
  int r;

  luaL_checktype(L, 2, LUA_TTABLE); /* options */
  memset(&opts, 0, sizeof(uv_process_options_t));

  argc = lua_objlen(L, 2);
  args = (char**)malloc((argc + 1) * sizeof(char*));
  args[0] = (char*)cmd;

  for (i = 1; i <= argc; i++) {
    lua_rawgeti(L, -1, i);

    args[i] = (char*)lua_tostring(L, -1);

    lua_pop(L, 1);
  }

  args[argc + 1] = NULL;

  cwd = NULL;
  lua_getfield(L, 2, "cwd");

  if (!lua_isnil(L, -1)) {
    cwd = (char*)lua_tostring(L, -1);
  }

  lua_pop(L, 1);

  env = NULL;
  lua_getfield(L, 2, "env");

  if (!lua_isnil(L, -1)) {
    int i, len;
    const char* key;
    const char* val;
    len = 32;
    env = (char**)malloc(32 * sizeof(char*));

    lua_pushnil(L);

    i = 0;

    while (lua_next(L, -2) != 0) {
      if (i >= len) {
        len *= 2;
        env = (char**)realloc(env, len * sizeof(char*));
      }

      key = lua_tostring(L, -2);
      val = lua_tostring(L, -1);

      lua_pushfstring(L, "%s=%s", key, val);

      env[i++] = (char*)lua_tostring(L, -1);

      lua_pop(L, 2);
    }

    env[i] = NULL;
  }

  lua_pop(L, 1);

  opts->exit_cb = _exit_cb;
  opts->file    = cmd;
  opts->args    = args;
  opts->env     = env;
  opts->cwd     = cwd;

  uv_stdio_container_t stdio[3];
  opts->stdio_count = 3;

  const char* stdfh_names[] = { "stdin", "stdout", "stderr" };

  for (i = 0; i < 3; i++) {
    lua_getfield(L, 2, stdfh_names[i]);

    if (lua_isnil(L, -1)) {
      stdio[i].flags = UV_IGNORE;
    } else {
      rava_object_t* obj = (rava_object_t*)luaL_checkudata(L, -1, RAVA_SOCKET_PIPE);
      stdio[i].flags = UV_INHERIT_STREAM;
      stdio[i].data.stream = &obj->h.stream;
    }

    lua_pop(L, 1);
  }

  opts->stdio = stdio;

  lua_getfield(L, 2, "detach");

  if (lua_toboolean(L, -1)) {
    opts->flags |= UV_PROCESS_DETACHED;
  } else {
    opts->flags = 0;
  }

  lua_pop(L, 1);

  rava_state_t*  curr = ravaL_state_self(L);
  rava_object_t* self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));

  luaL_getmetatable(L, RAVA_PROCESS_SPAWN);
  lua_setmetatable(L, -2);

  ravaL_object_init(curr, self);

  lua_insert(L, 1);
  lua_settop(L, 1);

  r = uv_spawn(ravaL_event_loop(L), &self->h.process, opts);

  free(args);

  if (env) free(env);

  if (r) {
    return luaL_error(L, uv_strerror(r));
  }

  if (opts->flags & UV_PROCESS_DETACHED) {
    uv_unref((uv_handle_t*)&self->h.process);

    return 1;
  } else {
    return ravaL_cond_wait(&self->rouse, curr);
  }
}

static int rava_spawn_kill(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_SPAWN);
  int signum = luaL_checkint(L, 2);
	int r = uv_process_kill(&self->h.process, signum);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  return 0;
}

static int rava_spawn_free(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  ravaL_object_close(self);
  return 0;
}

static int rava_spawn_tostring(lua_State* L)
{
  rava_object_t *self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_SPAWN);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_PROCESS_SPAWN, self);
  return 1;
}

luaL_Reg rava_spawn_meths[] = {
  {"kill",        rava_spawn_kill},
  {"__gc",        rava_spawn_free},
  {"__tostring",  rava_spawn_tostring},
  {NULL,          NULL}
};

LUA_API int luaopen_rava_process_spawn(lua_State* L)
{
  ravaL_class(L, RAVA_PROCESS_SPAWN, rava_spawn_meths);
  lua_pop(L, 1);
	lua_pushcfunction(L, rava_new_spawn);

  return 1;
}
