#ifndef RAVA_H
#define RAVA_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef __cplusplus
}
#endif

#include "uv.h"
#include "queue.h"

#undef RAVA_DEBUG

#ifdef RAVA_DEBUG
#  define TRACE(fmt, ...) do { \
    fprintf(stderr, "%s: %d: %s: " fmt, \
    __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
  } while (0)
#else
#  define TRACE(fmt, ...) ((void)0)
#endif /* RAVA_DEBUG */

typedef union rava_handle_u {
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
} rava_handle_t;

typedef union rava_req_u {
  uv_req_t          req;
  uv_write_t        write;
  uv_connect_t      connect;
  uv_shutdown_t     shutdown;
  uv_fs_t           fs;
  uv_work_t         work;
  uv_udp_send_t     udp_send;
  uv_getaddrinfo_t  getaddrinfo;
} rava_req_t;

/* registry table for rava object refs */
#define RAVA_REG_KEY "__RAVA__"

/* default buffer size for read operations */
#define RAVA_BUF_SIZE 4096

/* max path length */
#define RAVA_MAX_PATH 1024

/* state flags */
#define RAVA_STATE_START (1 << 0)
#define RAVA_STATE_READY (1 << 1)
#define RAVA_STATE_MAIN  (1 << 2)
#define RAVA_STATE_WAIT  (1 << 3)
#define RAVA_STATE_JOIN  (1 << 4)
#define RAVA_STATE_DEAD  (1 << 5)

/* rava states */
typedef struct rava_state_s  rava_state_t;
typedef struct rava_fiber_s  rava_fiber_t;
typedef struct rava_thread_s rava_thread_t;

typedef enum {
  RAVA_TFIBER,
  RAVA_TTHREAD
} rava_state_type;

#define RAVA_STATE_FIELDS \
  QUEUE         rouse; \
  QUEUE         queue; \
  QUEUE         join;  \
  QUEUE         cond;  \
  uv_loop_t*    loop;  \
  int           type;  \
  int           flags; \
  rava_state_t*  outer; \
  lua_State*    L;     \
  rava_req_t     req;   \
  void*         data

struct rava_state_s {
  RAVA_STATE_FIELDS;
};

struct rava_thread_s {
  RAVA_STATE_FIELDS;
  rava_state_t*    curr;
  uv_thread_t     tid;
  uv_async_t      async;
  uv_check_t      check;
};

struct rava_fiber_s {
  RAVA_STATE_FIELDS;
};

union rava_any_state {
  rava_state_t  state;
  rava_fiber_t  fiber;
  rava_thread_t thread;
};

/* rava objects */
#define RAVA_OSTARTED  (1 << 0)
#define RAVA_OSTOPPED  (1 << 1)
#define RAVA_OWAITING  (1 << 2)
#define RAVA_OCLOSING  (1 << 3)
#define RAVA_OCLOSED   (1 << 4)
#define RAVA_OSHUTDOWN (1 << 5)

#define ravaL_object_is_started(O)  ((O)->flags & RAVA_OSTARTED)
#define ravaL_object_is_stopped(O)  ((O)->flags & RAVA_OSTOPPED)
#define ravaL_object_is_waiting(O)  ((O)->flags & RAVA_OWAITING)
#define ravaL_object_is_closing(O)  ((O)->flags & RAVA_OCLOSING)
#define ravaL_object_is_shutdown(O) ((O)->flags & RAVA_OSHUTDOWN)
#define ravaL_object_is_closed(O)   ((O)->flags & RAVA_OCLOSED)

#define RAVA_OBJECT_FIELDS \
  QUEUE         rouse; \
  QUEUE         queue; \
  rava_state_t*  state; \
  int           flags; \
  int           type;  \
  int           count; \
  int           ref;   \
  void*         data

typedef struct rava_object_s {
  RAVA_OBJECT_FIELDS;
  rava_handle_t  h;
  uv_buf_t      buf;
} rava_object_t;

typedef struct rava_chan_s {
  RAVA_OBJECT_FIELDS;
  void*         put;
  void*         get;
} rava_chan_t;

union rava_any_object {
  rava_object_t object;
  rava_chan_t   chan;
};

typedef QUEUE rava_cond_t;

typedef struct rava_const_reg_s {
  const char*   key;
  int           val;
} rava_const_reg_t;

typedef struct rava_buf_t {
  size_t   size;
  uint8_t* head;
  uint8_t* base;
} rava_buf_t;

#ifdef WIN32
#  ifdef RAVA_EXPORT
#    define LUALIB_API __declspec(dllexport)
#  else
#    define LUALIB_API __declspec(dllimport)
#  endif
#else
#  define LUALIB_API LUA_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LUALIB_API int luaopen_rava(lua_State *L);

#ifdef __cplusplus
}
#endif

#define container_of(ptr, type, member) \
  ((type*) ((char*)(ptr) - offsetof(type, member)))

#define rava_boxpointer(L,u) \
    (*(void**) (lua_newuserdata(L, sizeof(void*))) = (u))
#define rava_unboxpointer(L,i) \
    (*(void**) (lua_touserdata(L, i)))

#define rava_boxinteger(L,n) \
    (*(lua_Integer*) (lua_newuserdata(L, sizeof(lua_Integer))) = (lua_Integer) (n))
#define rava_unboxinteger(L,i) \
    (*(lua_Integer*) (lua_touserdata(L, i)))

#endif /* RAVA_H */
