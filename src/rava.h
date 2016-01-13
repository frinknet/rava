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

/* metatables for various types */
#define RAVA_NS_T          "rava.ns"
#define RAVA_COND_T        "rava.cond"
#define RAVA_FIBER_T       "rava.fiber"
#define RAVA_THREAD_T      "rava.thread"
#define RAVA_ASYNC_T       "rava.async"
#define RAVA_TIMER_T       "rava.timer"
#define RAVA_IDLE_T        "rava.idle"
#define RAVA_FS_T          "rava.fs"
#define RAVA_FS_POLL_T     "rava.fs.poll"
#define RAVA_FILE_T        "rava.file"
#define RAVA_PIPE_T        "rava.pipe"
#define RAVA_TTY_T         "rava.tty"
#define RAVA_PROCESS_T     "rava.process"
#define RAVA_NET_TCP_T     "rava.net.tcp"
#define RAVA_NET_UDP_T     "rava.net.udp"

/* state flags */
#define RAVA_FSTART (1 << 0)
#define RAVA_FREADY (1 << 1)
#define RAVA_FMAIN  (1 << 2)
#define RAVA_FWAIT  (1 << 3)
#define RAVA_FJOIN  (1 << 4)
#define RAVA_FDEAD  (1 << 5)

/* ØMQ flags */
#define RAVA_ZMQ_SCLOSED (1 << 0)
#define RAVA_ZMQ_XDUPCTX (1 << 1)
#define RAVA_ZMQ_WSEND   (1 << 2)
#define RAVA_ZMQ_WRECV   (1 << 3)

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

int ravaL_traceback(lua_State *L);

uv_loop_t* ravaL_event_loop(lua_State* L);

int ravaL_state_in_thread(rava_state_t* state);
int ravaL_state_is_thread(rava_state_t* state);
int ravaL_state_is_active(rava_state_t* state);

void ravaL_state_ready  (rava_state_t* state);
int  ravaL_state_yield  (rava_state_t* state, int narg);
int  ravaL_state_suspend(rava_state_t* state);
int  ravaL_state_resume (rava_state_t* state, int narg);

void ravaL_fiber_ready  (rava_fiber_t* fiber);
int  ravaL_fiber_yield  (rava_fiber_t* fiber, int narg);
int  ravaL_fiber_suspend(rava_fiber_t* fiber);
int  ravaL_fiber_resume (rava_fiber_t* fiber, int narg);

int  ravaL_thread_loop   (rava_thread_t* thread);
int  ravaL_thread_once   (rava_thread_t* thread);
void ravaL_thread_ready  (rava_thread_t* thread);
int  ravaL_thread_yield  (rava_thread_t* thread, int narg);
int  ravaL_thread_suspend(rava_thread_t* thread);
int  ravaL_thread_resume (rava_thread_t* thread, int narg);
void ravaL_thread_enqueue(rava_thread_t* thread, rava_fiber_t* fiber);

rava_state_t*  ravaL_state_self (lua_State* L);
rava_thread_t* ravaL_thread_self(lua_State* L);

void ravaL_thread_init_main(lua_State* L);

rava_fiber_t*  ravaL_fiber_create (rava_state_t* outer, int narg);
rava_thread_t* ravaL_thread_create(rava_state_t* outer, int narg);

void ravaL_fiber_close (rava_fiber_t* self);

int  ravaL_thread_loop (rava_thread_t* self);
int  ravaL_thread_once (rava_thread_t* self);

void ravaL_object_init (rava_state_t* state, rava_object_t* self);
void ravaL_object_close(rava_object_t* self);

int  ravaL_stream_stop (rava_object_t* self);
void ravaL_stream_free (rava_object_t* self);
void ravaL_stream_close(rava_object_t* self);

typedef QUEUE rava_cond_t;

int ravaL_cond_init      (rava_cond_t* cond);
int ravaL_cond_wait      (rava_cond_t* cond, rava_state_t* curr);
int ravaL_cond_signal    (rava_cond_t* cond);
int ravaL_cond_broadcast (rava_cond_t* cond);

int ravaL_serialize_encode(lua_State* L, int narg);
int ravaL_serialize_decode(lua_State* L);

int ravaL_lib_decoder(lua_State* L);

void ravaL_alloc_cb   (uv_handle_t* handle, size_t size, uv_buf_t* buf);
void ravaL_connect_cb (uv_connect_t* conn, int status);

int ravaL_new_class (lua_State* L, const char* name, luaL_Reg* meths);
int ravaL_new_module(lua_State* L, const char* name, luaL_Reg* funcs);

typedef struct rava_const_reg_s {
  const char*   key;
  int           val;
} rava_const_reg_t;

extern luaL_Reg rava_thread_funcs[32];
extern luaL_Reg rava_thread_meths[32];

extern luaL_Reg rava_fiber_funcs[32];
extern luaL_Reg rava_fiber_meths[32];

extern luaL_Reg rava_cond_funcs[32];
extern luaL_Reg rava_cond_meths[32];

extern luaL_Reg rava_serialize_funcs[32];

extern luaL_Reg rava_timer_funcs[32];
extern luaL_Reg rava_timer_meths[32];

extern luaL_Reg rava_idle_funcs[32];
extern luaL_Reg rava_idle_meths[32];

extern luaL_Reg rava_system_fs_funcs[32];
extern luaL_Reg rava_system_file_meths[32];

extern luaL_Reg rava_stream_meths[32];

extern luaL_Reg rava_socket_funcs[32];

extern luaL_Reg rava_tcp_meths[32];
extern luaL_Reg rava_udp_meths[32];

extern luaL_Reg rava_pipe_funcs[32];
extern luaL_Reg rava_pipe_meths[32];

extern luaL_Reg rava_process_funcs[32];
extern luaL_Reg rava_process_meths[32];

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

/* lifted from luasys */
#define rava_boxpointer(L,u) \
    (*(void**) (lua_newuserdata(L, sizeof(void*))) = (u))
#define rava_unboxpointer(L,i) \
    (*(void**) (lua_touserdata(L, i)))

/* lifted from luasys */
#define rava_boxinteger(L,n) \
    (*(lua_Integer*) (lua_newuserdata(L, sizeof(lua_Integer))) = (lua_Integer) (n))
#define rava_unboxinteger(L,i) \
    (*(lua_Integer*) (lua_touserdata(L, i)))

#endif /* RAVA_H */
