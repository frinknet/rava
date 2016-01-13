#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "rava.h"

static int rava_system_file_stat(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, fstat, NULL, self->h.file);
}

static int rava_system_file_sync(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, fsync, NULL, self->h.file);
}

static int rava_system_file_datasync(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, fdatasync, NULL, self->h.file);
}

static int rava_system_file_truncate(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);
  off_t ofs = luaL_checkint(L, 2);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, ftruncate, NULL, self->h.file, ofs);
}

static int rava_system_file_utime(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, futime, NULL, self->h.file, atime, mtime);
}

static int rava_system_file_chmod(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, fchmod, NULL, self->h.file, mode);
}

static int rava_system_file_chown(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, fchown, NULL, self->h.file, uid, gid);
}

static int rava_system_file_read(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  size_t  len = luaL_optint(L, 2, RAVA_BUF_SIZE);
  int64_t ofs = luaL_optint(L, 3, -1);
  void*   buf = malloc(len); /* free from ctx->req.fs_req.data in cb */

  lua_settop(L, 0);
  RAVA_FS_CALL(L, read, buf, self->h.file, buf, len, ofs);
}

static int rava_system_file_write(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  size_t   len;
  void*    buf = (void*)luaL_checklstring(L, 2, &len);
  uint64_t ofs = luaL_optint(L, 3, 0);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, write, NULL, self->h.file, buf, len, ofs);
}

static int rava_system_file_close(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, close, NULL, self->h.file);
}

static int rava_system_file_free(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);

  if (self->data) free(self->data);

  return 0;
}

static int rava_system_file_tostring(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);

  lua_pushfstring(L, "userdata<%s>: %p", RAVA_FILE_T, self);

  return 1;
}

luaL_Reg rava_system_file_meths[] = {
  {"read",      rava_system_file_read},
  {"write",     rava_system_file_write},
  {"close",     rava_system_file_close},
  {"stat",      rava_system_file_stat},
  {"sync",      rava_system_file_sync},
  {"utime",     rava_system_file_utime},
  {"chmod",     rava_system_file_chmod},
  {"chown",     rava_system_file_chown},
  {"datasync",  rava_system_file_datasync},
  {"truncate",  rava_system_file_truncate},
  {"__gc",      rava_system_file_free},
  {"__tostring",rava_system_file_tostring},
  {NULL,        NULL}
};
