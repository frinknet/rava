#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "rava.h"

/* lifted from ravait */
static int _string_to_flags(lua_State* L, const char* str)
{
  if (strcmp(str, "r") == 0)
    return O_RDONLY;
  if (strcmp(str, "r+") == 0)
    return O_RDWR;
  if (strcmp(str, "w") == 0)
    return O_CREAT | O_TRUNC | O_WRONLY;
  if (strcmp(str, "w+") == 0)
    return O_CREAT | O_TRUNC | O_RDWR;
  if (strcmp(str, "a") == 0)
    return O_APPEND | O_CREAT | O_WRONLY;
  if (strcmp(str, "a+") == 0)
    return O_APPEND | O_CREAT | O_RDWR;

  return luaL_error(L, "Unknown file open flag: '%s'", str);
}

/* lifted from ravait */
static void _push_stats_table(lua_State* L, struct stat* s)
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

/* the rest of this file is mostly stolen from ravait - key difference is that
** we don't run Lua callbacks, but instead suspend and resume Lua threads */

static void rava_system_fs_result(lua_State* L, uv_fs_t* req)
{
  TRACE("enter fs result...\n");

  if (req->result < 0) {
    lua_pushnil(L);
    lua_pushinteger(L, (uv_errno_t)req->result);
	} else {
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

      case UV_FS_OPEN:
        {
          rava_object_t* self = (rava_object_t*)luaL_checkudata(L, -1, RAVA_FILE_T);
          self->h.file = req->result;
        }

        break;

      case UV_FS_READ:
        lua_pushinteger(L, req->result);
        lua_pushlstring(L, (const char*)req->data, req->result);
        free(req->data);

        req->data = NULL;

        break;

      case UV_FS_WRITE:
        lua_pushinteger(L, req->result);

        break;

      case UV_FS_READLINK:
        lua_pushstring(L, (char*)req->ptr);

        break;

      case UV_FS_SCANDIR:
        {
          int i;
          char* namep = (char*)req->ptr;
          int   count = req->result;

          lua_newtable(L);

          for (i = 1; i <= count; i++) {
            lua_pushstring(L, namep);
            lua_rawseti(L, -2, i);

            namep += strlen(namep) + 1; /* +1 for '\0' */
          }
        }

        break;

      case UV_FS_STAT:
      case UV_FS_LSTAT:
      case UV_FS_FSTAT:
        _push_stats_table(L, (struct stat*)req->ptr);

        break;

      default:
        luaL_error(L, "Unhandled fs_type");
    }
  }

  uv_fs_req_cleanup(req);
}

static void rava_system_fs_cb(uv_fs_t* req)
{
  rava_state_t* state = container_of(req, rava_state_t, req);
  rava_system_fs_result(state->L, req);
  ravaL_state_ready(state);
}

#define RAVA_FS_CALL(L, func, misc, ...) \
	do { \
    rava_state_t* curr = ravaL_state_self(L); \
    uv_loop_t*   loop = ravaL_event_loop(L); \
    uv_fs_t*     req; \
    uv_fs_cb     cb; \
    req = &curr->req.fs; \
		\
    if (curr->type == RAVA_TTHREAD) { \
      /* synchronous in main */ \
      cb = NULL; \
		} else { \
      cb = rava_system_fs_cb; \
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
    if (curr->type == RAVA_TTHREAD) { \
      rava_system_fs_result(L, req); \
			\
      return lua_gettop(L); \
		} else { \
      TRACE("suspending...\n"); \
			\
      return ravaL_state_suspend(curr); \
    } \
  } while(0)

static int rava_system_fs_open(lua_State* L)
{
  rava_state_t*  curr = ravaL_state_self(L);
  const char*   path = luaL_checkstring(L, 1);
  rava_object_t* self;

  int flags = _string_to_flags(L, luaL_checkstring(L, 2));
  int mode  = strtoul(luaL_checkstring(L, 3), NULL, 8);

  lua_settop(L, 0);

  self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));

  luaL_getmetatable(L, RAVA_FILE_T);
  lua_setmetatable(L, -2);
  ravaL_object_init(curr, self);

  self->h.file = -1; /* invalid file handle */

  RAVA_FS_CALL(L, open, NULL, path, flags, mode);
}

static int rava_system_fs_unlink(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, unlink, NULL, path);
}

static int rava_system_fs_mkdir(lua_State* L)
{
  const char*  path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, mkdir, NULL, path, mode);
}

static int rava_system_fs_rmdir(lua_State* L)
{
  const char*  path = luaL_checkstring(L, 1);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, rmdir, NULL, path);
}

static int rava_system_fs_scandir(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, scandir, NULL, path, 0);
}

static int rava_system_fs_stat(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, stat, NULL, path);
}

static int rava_system_fs_rename(lua_State* L)
{
  const char* old_path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, rename, NULL, old_path, new_path);
}

static int rava_system_fs_sendfile(lua_State* L)
{
  rava_object_t* o_file = (rava_object_t*)luaL_checkudata(L, 1, RAVA_FILE_T);
  rava_object_t* i_file = (rava_object_t*)luaL_checkudata(L, 2, RAVA_FILE_T);
  off_t  ofs = luaL_checkint(L, 3);
  size_t len = luaL_checkint(L, 4);

  lua_settop(L, 2);
  RAVA_FS_CALL(L, sendfile, NULL, o_file->h.file, i_file->h.file, ofs, len);
}

static int rava_system_fs_chmod(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, chmod, NULL, path, mode);
}

static int rava_system_fs_utime(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, utime, NULL, path, atime, mtime);
}

static int rava_system_fs_lstat(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, lstat, NULL, path);
}

static int rava_system_fs_link(lua_State* L)
{
  const char* src_path = luaL_checkstring(L, 1);
  const char* dst_path = luaL_checkstring(L, 2);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, link, NULL, src_path, dst_path);
}

static int rava_system_fs_symlink(lua_State* L)
{
  const char* src_path = luaL_checkstring(L, 1);
  const char* dst_path = luaL_checkstring(L, 2);
  int flags = _string_to_flags(L, luaL_checkstring(L, 3));

  lua_settop(L, 0);
  RAVA_FS_CALL(L, symlink, NULL, src_path, dst_path, flags);
}

static int rava_system_fs_readlink(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, readlink, NULL, path);
}

static int rava_system_fs_chown(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);

  lua_settop(L, 0);
  RAVA_FS_CALL(L, chown, NULL, path, uid, gid);
}

static int rava_system_fs_cwd(lua_State* L)
{
  char dir[RAVA_MAX_PATH];
  int r = uv_cwd(dir, (size_t*)RAVA_MAX_PATH);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  lua_pushstring(L, dir);

  return 1;
}

static int rava_system_fs_chdir(lua_State* L)
{
  const char* dir = luaL_checkstring(L, 1);
  int r = uv_chdir(dir);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  return 0;
}

static int rava_system_fs_exepath(lua_State* L) {
  char buffer[RAVA_MAX_PATH];
  size_t len = RAVA_MAX_PATH;

  uv_exepath(buffer, &len);
  lua_pushlstring(L, buffer, len);

  return 1;
}

static int rava_system_fs_home(lua_State* L)
{
  char dir[RAVA_MAX_PATH];
  int r = uv_os_homedir(dir, (size_t*)RAVA_MAX_PATH);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  lua_pushstring(L, dir);

  return 1;
}

luaL_Reg rava_system_fs_funcs[] = {
  {"open",        rava_system_fs_open},
  {"unlink",      rava_system_fs_unlink},
  {"mkdir",       rava_system_fs_mkdir},
  {"rmdir",       rava_system_fs_rmdir},
  {"scandir",     rava_system_fs_scandir},
  {"stat",        rava_system_fs_stat},
  {"rename",      rava_system_fs_rename},
  {"sendfile",    rava_system_fs_sendfile},
  {"chmod",       rava_system_fs_chmod},
  {"chown",       rava_system_fs_chown},
  {"utime",       rava_system_fs_utime},
  {"lstat",       rava_system_fs_lstat},
  {"link",        rava_system_fs_link},
  {"symlink",     rava_system_fs_symlink},
  {"readlink",    rava_system_fs_readlink},
  {"cwd",         rava_system_fs_cwd},
  {"chdir",       rava_system_fs_chdir},
  {"exepath",     rava_system_fs_exepath},
  {"home",        rava_system_fs_home},
  {NULL,          NULL}
};
