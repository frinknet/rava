#include "rava_fs.h"
#include "rava_agent.h"

/* shamelessly stolen from luvit - bugs are my own */
static int string_to_flags(lua_State* L, const char* str)
{
  if(strcmp(str, "r") == 0)
    return O_RDONLY;

  if(strcmp(str, "r+") == 0)
    return O_RDWR;

  if(strcmp(str, "w") == 0)
    return O_CREAT | O_TRUNC | O_WRONLY;

  if(strcmp(str, "w+") == 0)
    return O_CREAT | O_TRUNC | O_RDWR;

  if(strcmp(str, "a") == 0)
    return O_APPEND | O_CREAT | O_WRONLY;

  if(strcmp(str, "a+") == 0)
    return O_APPEND | O_CREAT | O_RDWR;

  return luaL_error(L, "Unknown file open flag: '%s'", str);
}

/* shamelessly stolen from luvit - bugs are my own */
static void push_stats_table(lua_State* L, struct stat* s)
{
  lua_newtable(L);
  lua_pushinteger(L, s->st_dev);
  lua_setfield(L, -2, "dev");
  lua_pushinteger(L, s->st_ino);
  lua_setfield(L, -2, "ino");
  lua_pushinteger(L, s->st_mode);
  lua_setfield(L, -2, "mode");
  lua_pushinteger(L, s->st_nlink);
  lua_setfield(L, -2, "nlink");
  lua_pushinteger(L, s->st_uid);
  lua_setfield(L, -2, "uid");
  lua_pushinteger(L, s->st_gid);
  lua_setfield(L, -2, "gid");
  lua_pushinteger(L, s->st_rdev);
  lua_setfield(L, -2, "rdev");
  lua_pushinteger(L, s->st_size);
  lua_setfield(L, -2, "size");
#ifdef __POSIX__
  lua_pushinteger(L, s->st_blksize);
  lua_setfield(L, -2, "blksize");
  lua_pushinteger(L, s->st_blocks);
  lua_setfield(L, -2, "blocks");
#endif
  lua_pushinteger(L, s->st_atime);
  lua_setfield(L, -2, "atime");
  lua_pushinteger(L, s->st_mtime);
  lua_setfield(L, -2, "mtime");
  lua_pushinteger(L, s->st_ctime);
  lua_setfield(L, -2, "ctime");
#ifndef _WIN32
  lua_pushboolean(L, S_ISREG(s->st_mode));
  lua_setfield(L, -2, "is_file");
  lua_pushboolean(L, S_ISDIR(s->st_mode));
  lua_setfield(L, -2, "is_directory");
  lua_pushboolean(L, S_ISCHR(s->st_mode));
  lua_setfield(L, -2, "is_character_device");
  lua_pushboolean(L, S_ISBLK(s->st_mode));
  lua_setfield(L, -2, "is_block_device");
  lua_pushboolean(L, S_ISFIFO(s->st_mode));
  lua_setfield(L, -2, "is_fifo");
  lua_pushboolean(L, S_ISLNK(s->st_mode));
  lua_setfield(L, -2, "is_symbolic_link");
  lua_pushboolean(L, S_ISSOCK(s->st_mode));
  lua_setfield(L, -2, "is_socket");
#endif
}

/* mostly stolen from luvit - bugs are my own */
static void fs_result(lua_State* L, uv_fs_t* req)
{
  if (req->result < 0) {
    lua_pushnil(L);
    lua_pushinteger(L, (uv_errno_t)req->result);
  }
  else {
    switch (req->fs_type) {
      case UV_FS_RENAME:
      case UV_FS_UNLINK:
      case UV_FS_RMDIR:
      case UV_FS_MKDIR:
      case UV_FS_FSYNC:
      case UV_FS_FTRUNCATE:
      case UV_FS_FDATASYNC:
      case UV_FS_LINK:
      case UV_FS_SYMLINK:
      case UV_FS_CHMOD:
      case UV_FS_FCHMOD:
      case UV_FS_CHOWN:
      case UV_FS_FCHOWN:
      case UV_FS_UTIME:
      case UV_FS_FUTIME:
      case UV_FS_CLOSE:
        lua_pushinteger(L, req->result);
        break;

      case UV_FS_OPEN: {
        ray_agent_t* self = ray_agent_new(L);
        luaL_getmetatable(L, "ray.file");
        lua_setmetatable(L, -2);
        self->h.file = req->result;
        break;
      }

      case UV_FS_READ: {
        lua_pushinteger(L, req->result);
        lua_pushlstring(L, (const char*)req->data, req->result);
        free(req->data);
        req->data = NULL;
        break;
      }

      case UV_FS_WRITE:
        lua_pushinteger(L, req->result);
        break;

      case UV_FS_READLINK:
        lua_pushstring(L, (char*)req->ptr);
        break;

      case UV_FS_SCANDIR: {
        int i;
        char* namep = (char*)req->ptr;
        int   count = req->result;
        lua_newtable(L);
        for (i = 1; i <= count; i++) {
          lua_pushstring(L, namep);
          lua_rawseti(L, -2, i);
          namep += strlen(namep) + 1; /* +1 for '\0' */
        }
        break;
      }

      case UV_FS_STAT:
      case UV_FS_LSTAT:
      case UV_FS_FSTAT:
        push_stats_table(L, (struct stat*)req->ptr);
        break;

      default:
        luaL_error(L, "Unhandled fs_type");
    }
  }
  uv_fs_req_cleanup(req);
}

void ray_fs_cb(uv_fs_t* req)
{
  ray_fiber_t* curr = container_of(req, ray_fiber_t, r);
  fs_result(curr->L, req);
  ray_resume(curr, LUA_MULTRET);
}

int ray_fs_open(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  int flags = string_to_flags(L, luaL_checkstring(L, 2));
  int mode  = strtoul(luaL_checkstring(L, 3), NULL, 8);
  lua_settop(L, 0);
  RAY_FS_CALL(L, open, NULL, path, flags, mode);
}

int ray_fs_unlink(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_settop(L, 0);
  RAY_FS_CALL(L, unlink, NULL, path);
}

int ray_fs_mkdir(lua_State* L)
{
  const char*  path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  lua_settop(L, 0);
  RAY_FS_CALL(L, mkdir, NULL, path, mode);
}

int ray_fs_rmdir(lua_State* L)
{
  const char*  path = luaL_checkstring(L, 1);
  lua_settop(L, 0);
  RAY_FS_CALL(L, rmdir, NULL, path);
}

int ray_fs_scandir(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_settop(L, 0);
  RAY_FS_CALL(L, scandir, NULL, path, 0);
}

int ray_fs_stat(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_settop(L, 0);
  RAY_FS_CALL(L, stat, NULL, path);
}

int ray_fs_rename(lua_State* L)
{
  const char* old_path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  lua_settop(L, 0);
  RAY_FS_CALL(L, rename, NULL, old_path, new_path);
}

int ray_fs_sendfile(lua_State* L)
{
  ray_agent_t* o_file = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  ray_agent_t* i_file = (ray_agent_t*)luaL_checkudata(L, 2, "ray.file");

  off_t  ofs = luaL_checkint(L, 3);
  size_t len = luaL_checkint(L, 4);
  lua_settop(L, 2);

  RAY_FS_CALL(L, sendfile, NULL, o_file->h.file, i_file->h.file, ofs, len);
}

int ray_fs_chmod(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  lua_settop(L, 0);
  RAY_FS_CALL(L, chmod, NULL, path, mode);
}

int ray_fs_utime(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);
  lua_settop(L, 0);
  RAY_FS_CALL(L, utime, NULL, path, atime, mtime);
}

int ray_fs_lstat(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_settop(L, 0);
  RAY_FS_CALL(L, lstat, NULL, path);
}

int ray_fs_link(lua_State* L)
{
  const char* src_path = luaL_checkstring(L, 1);
  const char* dst_path = luaL_checkstring(L, 2);
  lua_settop(L, 0);
  RAY_FS_CALL(L, link, NULL, src_path, dst_path);
}

int ray_fs_symlink(lua_State* L)
{
  const char* src_path = luaL_checkstring(L, 1);
  const char* dst_path = luaL_checkstring(L, 2);
  int flags = string_to_flags(L, luaL_checkstring(L, 3));
  lua_settop(L, 0);
  RAY_FS_CALL(L, symlink, NULL, src_path, dst_path, flags);
}

int ray_fs_readlink(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_settop(L, 0);
  RAY_FS_CALL(L, readlink, NULL, path);
}

int ray_fs_chown(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);
  lua_settop(L, 0);
  RAY_FS_CALL(L, chown, NULL, path, uid, gid);
}

int ray_fs_cwd(lua_State* L)
{
  char buf[RAY_MAX_PATH];
  int err = uv_cwd(buf, (size_t*) RAY_MAX_PATH);
  if (err) {
    return luaL_error(L, uv_strerror(err));
  }
  lua_pushstring(L, buf);
  return 1;
}

int ray_fs_chdir(lua_State* L)
{
  const char* dir = luaL_checkstring(L, 1);
  int err = uv_chdir(dir);
  if (err) {
    return luaL_error(L, uv_strerror(err));
  }
  return 0;
}

int ray_fs_exepath(lua_State* L)
{
  char buf[RAY_MAX_PATH];
  size_t len = RAY_MAX_PATH;
  uv_exepath(buf, &len);
  lua_pushlstring(L, buf, len);
  return 1;
}

/* file instance methods */
int ray_file_stat(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  lua_settop(L, 0);
  RAY_FS_CALL(L, fstat, NULL, self->h.file);
}

int ray_file_sync(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  lua_settop(L, 0);
  RAY_FS_CALL(L, fsync, NULL, self->h.file);
}

int ray_file_datasync(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  lua_settop(L, 0);
  RAY_FS_CALL(L, fdatasync, NULL, self->h.file);
}

int ray_file_truncate(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  off_t ofs = luaL_checkint(L, 2);
  lua_settop(L, 0);
  RAY_FS_CALL(L, ftruncate, NULL, self->h.file, ofs);
}

int ray_file_utime(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);
  lua_settop(L, 0);
  RAY_FS_CALL(L, futime, NULL, self->h.file, atime, mtime);
}

int ray_file_chmod(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  lua_settop(L, 0);
  RAY_FS_CALL(L, fchmod, NULL, self->h.file, mode);
}

int ray_file_chown(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);
  lua_settop(L, 0);
  RAY_FS_CALL(L, fchown, NULL, self->h.file, uid, gid);
}

int ray_file_read(lua_State *L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");

  size_t  len = luaL_optint(L, 2, RAY_BUFFER_SIZE);
  int64_t ofs = luaL_optint(L, 3, -1);
  void*   buf = malloc(len); /* free from ctx->r.fs_req.data in cb */

  lua_settop(L, 0);
  RAY_FS_CALL(L, read, buf, self->h.file, buf, len, ofs);
}

int ray_file_write(lua_State *L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");

  size_t   len;
  void*    buf = (void*)luaL_checklstring(L, 2, &len);
  uint64_t ofs = luaL_optint(L, 3, 0);

  lua_settop(L, 0);
  RAY_FS_CALL(L, write, NULL, self->h.file, buf, len, ofs);
}

int ray_file_close(lua_State *L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, "ray.file");
  lua_settop(L, 0);
  RAY_FS_CALL(L, close, NULL, self->h.file);
}
