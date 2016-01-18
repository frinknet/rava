#include "rava.h"
#include "rava_core.h"
#include "rava_socket.h"

int rava_new_pipe(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));
  luaL_getmetatable(L, RAVA_SOCKET_PIPE);
  lua_setmetatable(L, -2);
  int ipc = 0;
  if (!lua_isnoneornil(L, 2)) {
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    ipc = lua_toboolean(L, 2);
  }
  uv_pipe_init(ravaL_event_loop(L), &self->h.pipe, ipc);
  return 1;
}

static int rava_pipe_open(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_PIPE);
  uv_file fh;
  if (lua_isuserdata(L, 2)) {
    rava_object_t* file = (rava_object_t*)luaL_checkudata(L, 2, RAVA_SYSTEM_FILE);
    fh = file->h.file;
  }
  else {
    fh = luaL_checkint(L, 2);
  }
  uv_pipe_open(&self->h.pipe, fh);
  return 0;
}

static int rava_pipe_bind(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_PIPE);
  const char*   path = luaL_checkstring(L, 2);
	int r = uv_pipe_bind(&self->h.pipe, path);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  return 0;
}

static int rava_pipe_connect(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_PIPE);
  const char*   path = luaL_checkstring(L, 2);
  rava_state_t*  curr = ravaL_state_self(L);

  uv_pipe_connect(&curr->req.connect, &self->h.pipe, path, ravaL_connect_cb);

  return 0;
}

static int rava_pipe_tostring(lua_State *L)
{
  rava_object_t *self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_SOCKET_PIPE);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_SOCKET_PIPE, self);
  return 1;
}

luaL_Reg rava_pipe_meths[] = {
  {"open",        rava_pipe_open},
  {"bind",        rava_pipe_bind},
  {"connect",     rava_pipe_connect},
  {"__tostring",  rava_pipe_tostring},
  {NULL,          NULL}
};

LUA_API int luaopen_rava_socket_pipe(lua_State* L)
{
  ravaL_class(L, RAVA_SOCKET_PIPE, rava_stream_meths);
  luaL_register(L, NULL, rava_pipe_meths);
  lua_pop(L, 1);

  lua_pushcfunction(L, rava_new_pipe);

  return 1;
}
