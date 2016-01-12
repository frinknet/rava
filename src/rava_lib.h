#ifndef _RAY_LIB_H_
#define _RAY_LIB_H_

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef WIN32
# define UNUSED /* empty */
# define INLINE __inline
#else
# define UNUSED __attribute__((unused))
# define INLINE inline
#endif

#undef RAY_DEBUG
#define NGX_DEBUG

#include "queue.h"
#include "uv.h"

#ifdef RAY_DEBUG
#  define TRACE(fmt, ...) do { \
    fprintf(stderr, "%s: %d: %s: " fmt, \
    __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
  } while (0)
#else
#  define TRACE(fmt, ...) ((void)0)
#endif /* RAY_DEBUG */

typedef union ray_handle_u {
  uv_handle_t     handle;
  uv_stream_t     stream;
  uv_tcp_t        tcp;
  uv_pipe_t       pipe;
  uv_prepare_t    prepare;
  uv_check_t      check;
  uv_idle_t       idle;
  uv_async_t      async;
  uv_timer_t      timer;
  uv_fs_event_t   fs_event;
  uv_fs_poll_t    fs_poll;
  uv_poll_t       poll;
  uv_process_t    process;
  uv_tty_t        tty;
  uv_udp_t        udp;
  uv_file         file;
} ray_handle_t;

typedef union ray_req_u {
  uv_req_t          req;
  uv_write_t        write;
  uv_connect_t      connect;
  uv_shutdown_t     shutdown;
  uv_fs_t           fs;
  uv_work_t         work;
  uv_udp_send_t     udp_send;
  uv_getaddrinfo_t  getaddrinfo;
} ray_req_t;

/* default buffer size for read operations */
#define RAY_BUFFER_SIZE 4096

/* max path length */
#define RAY_MAX_PATH 1024

#define container_of(ptr, type, member) \
  ((type*) ((char*)(ptr) - offsetof(type, member)))

/* lifted from luasys */
#define ray_boxpointer(L,u) \
    (*(void**) (lua_newuserdata(L, sizeof(void*))) = (u))
#define ray_unboxpointer(L,i) \
    (*(void**) (lua_touserdata(L, i)))

/* lifted from luasys */
#define ray_boxinteger(L,n) \
    (*(lua_Integer*) (lua_newuserdata(L, sizeof(lua_Integer))) = (lua_Integer)(n))
#define ray_unboxinteger(L,i) \
    (*(lua_Integer*) (lua_touserdata(L, i)))

#define ray_absindex(L, i) \
  ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

#define LUA_MODULE(module, L) \
  loaopen_#module(L)


#define RAY_STARTED 1
#define RAY_CLOSED  2
#define RAY_RUNNING 4

#define RAY_MODULE         "ray"
#define RAY_MODULE_FS      "ray_fs"
#define RAY_MODULE_SYSTEM  "ray_system"

#define RAY_CLASS_FIBER    "ray.fiber"
#define RAY_CLASS_FILE     "ray.file"
#define RAY_CLASS_PIPE     "ray.pipe"
#define RAY_CLASS_TCP      "ray.tcp"
#define RAY_CLASS_TIMER    "ray.timer"
#define RAY_CLASS_UDP      "ray.udp"

typedef struct ray_fiber_s  ray_fiber_t;
typedef struct ray_agent_s  ray_agent_t;
typedef struct ray_evt_s    ray_evt_t;
typedef struct ray_evq_s    ray_evq_t;
typedef struct ray_buf_s    ray_buf_t;

struct ray_buf_s {
  size_t  size;
  size_t  offs;
  char*   base;
};

typedef enum {
  RAY_UNKNOWN = -1,
  RAY_CUSTOM,
  RAY_ERROR,
  RAY_READ,
  RAY_WRITE,
  RAY_CLOSE,
  RAY_CONNECTION,
  RAY_TIMER,
  RAY_IDLE,
  RAY_CONNECT,
  RAY_SHUTDOWN,
  RAY_WORK,
  RAY_FS_CUSTOM,
  RAY_FS_ERROR,
  RAY_FS_OPEN,
  RAY_FS_CLOSE,
  RAY_FS_READ,
  RAY_FS_WRITE,
  RAY_FS_SENDFILE,
  RAY_FS_STAT,
  RAY_FS_LSTAT,
  RAY_FS_FSTAT,
  RAY_FS_FTRUNCATE,
  RAY_FS_UTIME,
  RAY_FS_FUTIME,
  RAY_FS_CHMOD,
  RAY_FS_FCHMOD,
  RAY_FS_FSYNC,
  RAY_FS_FDATASYNC,
  RAY_FS_UNLINK,
  RAY_FS_RMDIR,
  RAY_FS_MKDIR,
  RAY_FS_RENAME,
  RAY_FS_READDIR,
  RAY_FS_LINK,
  RAY_FS_SYMLINK,
  RAY_FS_READLINK,
  RAY_FS_CHOWN,
  RAY_FS_FCHOWN
} ray_type_t;

struct ray_evt_s {
  ray_type_t    type;
  int           info;
  void*         data;
};

struct ray_evq_s {
  size_t        nput;
  size_t        nget;
  size_t        size;
  ray_evt_t*    evts;
};

struct ray_agent_s {
  ray_handle_t  h;
  ray_buf_t    buf;
  int           flags;
  int           count;
  QUEUE   fiber_queue;
  QUEUE   queue;
  void*         data;
  int           ref;
};

struct ray_fiber_s {
  ray_req_t     r;
  lua_State*    L;
  int           L_ref;
  int           flags;
  uv_idle_t     hook;
  QUEUE   fiber_queue;
  QUEUE   queue;
  int           ref;
};

static ray_fiber_t* RAY_MAIN;
static uv_async_t   RAY_PUMP;

int rayL_traceback(lua_State* L);
int rayL_require(lua_State* L, const char* path);
int rayL_lib_decoder(lua_State* L);
int rayL_lib_encoder(lua_State* L);
int rayL_module(lua_State* L, const char* name, luaL_Reg* funcs);
int rayL_class(lua_State* L, const char* name, luaL_Reg* meths);
int rayL_extend(lua_State* L, const char* base, const char* name, luaL_Reg* meths);
void* rayL_checkudata(lua_State* L, int idx, const char* name);
void rayL_dump_stack(lua_State* L);

#endif
