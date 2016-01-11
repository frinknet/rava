#include "rava_lib.h"

#define RAY_FS_CALL(L, func, misc, ...) do { \
    ray_fiber_t* curr = ray_current(L); \
    uv_loop_t*   loop = uv_default_loop(); \
    uv_fs_t*     req  = &curr->r.fs; \
    req->data   = misc; \
    uv_fs_cb cb = (curr == RAY_MAIN) ? NULL : ray_fs_cb ; \
    int rc = uv_fs_##func(loop, req, __VA_ARGS__, cb); \
    if (rc < 0) return ray_push_error(L, rc); \
    if (cb) return ray_suspend(curr); \
    else { \
      fs_result(L, req); \
      return lua_gettop(L); \
    } \
  } while(0)

static int string_to_flags(lua_State* L, const char* str);
static void push_stats_table(lua_State* L, struct stat* s);
static void fs_result(lua_State* L, uv_fs_t* req);

void ray_fs_cb(uv_fs_t* req);
int ray_fs_open(lua_State* L);
int ray_fs_unlink(lua_State* L);
int ray_fs_mkdir(lua_State* L);
int ray_fs_rmdir(lua_State* L);
int ray_fs_scandir(lua_State* L);
int ray_fs_stat(lua_State* L);
int ray_fs_rename(lua_State* L);
int ray_fs_sendfile(lua_State* L);
int ray_fs_chmod(lua_State* L);
int ray_fs_utime(lua_State* L);
int ray_fs_lstat(lua_State* L);
int ray_fs_link(lua_State* L);
int ray_fs_symlink(lua_State* L);
int ray_fs_readlink(lua_State* L);
int ray_fs_chown(lua_State* L);
int ray_fs_cwd(lua_State* L);
int ray_fs_chdir(lua_State* L);
int ray_fs_exepath(lua_State* L);

int ray_file_stat(lua_State* L);
int ray_file_sync(lua_State* L);
int ray_file_datasync(lua_State* L);
int ray_file_truncate(lua_State* L);
int ray_file_utime(lua_State* L);
int ray_file_chmod(lua_State* L);
int ray_file_chown(lua_State* L);
int ray_file_read(lua_State *L);
int ray_file_write(lua_State *L);
int ray_file_close(lua_State *L);
