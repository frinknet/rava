#include "rava.h"
#include "rava_core_stream.h"
#include "rava_core_state.h"

static void _read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
  TRACE("got data\n");

  rava_object_t* self = container_of(stream, rava_object_t, h);

  if (QUEUE_EMPTY(&self->rouse)) {
    TRACE("empty read queue, save buffer and stop read\n");

    ravaL_stream_stop(self);

    if (nread >= 0) {
      self->buf   = uv_buf_init(buf->base, buf->len);
      self->count = nread;
		} else {
      if (buf->base) {
        free(buf->base);
      }
    }
	} else {
    TRACE("have states waiting...\n");

    QUEUE* q;
    rava_state_t* s;

    q = QUEUE_HEAD(&self->rouse);
    s = QUEUE_DATA(q, rava_state_t, cond);

    QUEUE_REMOVE(q);

    TRACE("data - len: %i\n", (int)len);

    lua_settop(s->L, 0);

    if (nread >= 0) {
      lua_pushinteger(s->L, nread);
      lua_pushlstring(s->L, (char*)buf->base, buf->len);

      if (nread == 0) ravaL_stream_stop(self);
		} else if (nread == UV_EOF) {
      TRACE("GOT EOF\n");

      lua_settop(s->L, 0);
      lua_pushnil(s->L);
		} else {
      lua_settop(s->L, 0);
      lua_pushboolean(s->L, 0);
      lua_pushfstring(s->L, "read: %s", uv_strerror(nread));

      TRACE("READ ERROR, CLOSING STREAM\n");

      ravaL_stream_stop(self);
      ravaL_object_close(self);
    }

    if (buf->base) {
      free(buf->base);
    }

    TRACE("wake up state: %p\n", s);
    ravaL_state_ready(s);
  }
}

static void _write_cb(uv_write_t* req, int status)
{
  rava_state_t* rouse = container_of(req, rava_state_t, req);
  lua_settop(rouse->L, 0);
  lua_pushinteger(rouse->L, status);

  TRACE("write_cb - wake up: %p\n", rouse);

  ravaL_state_ready(rouse);
}

static void _shutdown_cb(uv_shutdown_t* req, int status)
{
  rava_object_t* self = container_of(req->handle, rava_object_t, h);
  rava_state_t* state = container_of(req, rava_state_t, req);
  lua_settop(state->L, 0);
  lua_pushinteger(state->L, status);
  ravaL_cond_signal(&self->rouse);
}

static void _listen_cb(uv_stream_t* server, int status)
{
  TRACE("got client connection...\n");

  rava_object_t* self = container_of(server, rava_object_t, h);

  if (ravaL_object_is_waiting(self)) {
    QUEUE* q = QUEUE_HEAD(&self->rouse);

    rava_state_t* s = QUEUE_DATA(q, rava_state_t, cond);
    lua_State* L = s->L;

    TRACE("is waiting..., lua_State*: %p\n", L);

    luaL_checktype(L, 2, LUA_TUSERDATA);
    rava_object_t* conn = (rava_object_t*)lua_touserdata(L, 2);

    TRACE("got client conn: %p\n", conn);

    ravaL_object_init(s, conn);

    int r = uv_accept(&self->h.stream, &conn->h.stream);

    TRACE("accept returned ok\n");

    if (r) {
      TRACE("ERROR: %s\n", uv_strerror(r));

      lua_settop(L, 0);
      lua_pushnil(L);
      lua_pushstring(L, uv_strerror(r));
    }

    self->flags &= ~RAVA_OWAITING;

    ravaL_cond_signal(&self->rouse);
	} else {
    TRACE("increment backlog count\n");

    self->count++;
  }
}

/* used by udp and stream */
void ravaL_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
  rava_object_t* self = container_of(handle, rava_object_t, h);
	size = (size_t)self->buf.len;

  uv_buf_init((char*)malloc(size), size);
}

/* used by tcp and pipe */
void ravaL_connect_cb(uv_connect_t* req, int status)
{
  rava_state_t* state = container_of(req, rava_state_t, req);

  ravaL_state_ready(state);
}

void ravaL_stream_free(rava_object_t* self)
{
  ravaL_object_close(self);

  TRACE("free stream: %p\n", self);

  if (self->buf.base) {
    free(self->buf.base);

    self->buf.base = NULL;
    self->buf.len  = 0;
  }
}

void ravaL_stream_close(rava_object_t* self)
{
  TRACE("close stream\n");

  if (ravaL_object_is_started(self)) {
    ravaL_stream_stop(self);
  }

  ravaL_object_close(self);

  if (self->buf.base) {
    free(self->buf.base);

    self->buf.base = NULL;
    self->buf.len  = 0;
  }
}

int ravaL_stream_start(rava_object_t* self)
{
  if (!ravaL_object_is_started(self)) {
    self->flags |= RAVA_OSTARTED;

    return uv_read_start(&self->h.stream, ravaL_alloc_cb, _read_cb);
  }

  return 0;
}

int ravaL_stream_stop(rava_object_t* self)
{
  if (ravaL_object_is_started(self)) {
    self->flags &= ~RAVA_OSTARTED;

    return uv_read_stop(&self->h.stream);
  }

  return 0;
}

static int rava_stream_listen(lua_State* L)
{
  luaL_checktype(L, 1, LUA_TUSERDATA);
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);

  int backlog = luaL_optinteger(L, 2, 128);
	int r = uv_listen(&self->h.stream, backlog, _listen_cb);

  if (r < 0) {
    TRACE("listen error\n");

    return luaL_error(L, "listen: %s", uv_strerror(r));
  }

  return 0;
}

static int rava_stream_accept(lua_State *L)
{
  luaL_checktype(L, 1, LUA_TUSERDATA);
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  rava_object_t* conn = (rava_object_t*)lua_touserdata(L, 2);

  rava_state_t* curr = ravaL_state_self(L);

  if (self->count) {
    self->count--;

    int r = uv_accept(&self->h.stream, &conn->h.stream);

    if (r < 0) {
      lua_settop(L, 0);
      lua_pushnil(L);
      lua_pushstring(L, uv_strerror(r));

      return 2;
    }

    return 1;
  }

  self->flags |= RAVA_OWAITING;

  return ravaL_cond_wait(&self->rouse, curr);
}

// #define STREAM_ERROR(L,fmt,loop) do { \
//   int r = uv_last_error(loop); \
//   lua_settop(L, 0); \
//   lua_pushboolean(L, 0); \
//   lua_pushfstring(L, fmt, uv_strerror(r)); \
//   TRACE("STREAM ERROR: %s\n", lua_tostring(L, -1)); \
// } while (0)

static int rava_stream_start(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);

  if (ravaL_stream_start(self)) {
    ravaL_stream_stop(self);
    ravaL_object_close(self);

    // STREAM_ERROR(L, "read start: %s", ravaL_event_loop(L));

    // return 2;
		return 0;
  }

  lua_pushboolean(L, 1);

  return 1;
}

static int rava_stream_stop(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);

  if (ravaL_stream_stop(self)) {
    // STREAM_ERROR(L, "read stop: %s", ravaL_event_loop(L));

    // return 2;
		return 0;
  }

  lua_pushboolean(L, 1);

  return 1;
}

static int rava_stream_read(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  rava_state_t*  curr = ravaL_state_self(L);
  int len = luaL_optinteger(L, 2, 4096);

  if (ravaL_object_is_closing(self)) {
    TRACE("error: reading from closed stream\n");

    lua_pushnil(L);
    lua_pushstring(L, "attempt to read from a closed stream");

    return 2;
  }

  if (self->buf.base) {
    /* we have a buffer use that */
    TRACE("have pending data\n");

    lua_pushinteger(L, self->count);
    lua_pushlstring(L, (char*)self->buf.base, self->count);

    free(self->buf.base);

    self->buf.base = NULL;
    self->buf.len  = 0;
    self->count    = 0;

    return 2;
  }

  self->buf.len = len;

  if (!ravaL_object_is_started(self)) {
    ravaL_stream_start(self);
  }

  TRACE("read called... waiting\n");

  return ravaL_cond_wait(&self->rouse, curr);
}

static int rava_stream_write(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);

  size_t len;
  const char* chunk = luaL_checklstring(L, 2, &len);

  uv_buf_t buf = uv_buf_init((char*)chunk, len);

  rava_state_t* curr = ravaL_state_self(L);
  uv_write_t*  req  = &curr->req.write;

  if (uv_write(req, &self->h.stream, &buf, 1, _write_cb)) {
    ravaL_stream_stop(self);
    ravaL_object_close(self);

    // STREAM_ERROR(L, "write: %s", ravaL_event_loop(L));

    // return 2;
		return 0;
  }

  lua_settop(curr->L, 1);

  return ravaL_state_suspend(curr);
}

static int rava_stream_shutdown(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);

  if (!ravaL_object_is_shutdown(self)) {
    self->flags |= RAVA_OSHUTDOWN;
    rava_state_t* curr = ravaL_state_self(L);

    uv_shutdown(&curr->req.shutdown, &self->h.stream, _shutdown_cb);

    return ravaL_cond_wait(&self->rouse, curr);
  }

  return 1;
}

static int rava_stream_readable(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  lua_pushboolean(L, uv_is_readable(&self->h.stream));

  return 1;
}

static int rava_stream_writable(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  lua_pushboolean(L, uv_is_writable(&self->h.stream));

  return 1;
}

static int rava_stream_close(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  ravaL_stream_close(self);

  return ravaL_cond_wait(&self->rouse, ravaL_state_self(L));
}

static int rava_stream_free(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  ravaL_stream_free(self);

  return 1;
}

static int rava_stream_tostring(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  lua_pushfstring(L, "userdata<rava.stream>: %p", self);

  return 1;
}

luaL_Reg rava_stream_meths[] = {
  {"read",      rava_stream_read},
  {"readable",  rava_stream_readable},
  {"write",     rava_stream_write},
  {"writable",  rava_stream_writable},
  {"start",     rava_stream_start},
  {"stop",      rava_stream_stop},
  {"listen",    rava_stream_listen},
  {"accept",    rava_stream_accept},
  {"shutdown",  rava_stream_shutdown},
  {"close",     rava_stream_close},
  {"__gc",      rava_stream_free},
  {"__tostring",rava_stream_tostring},
  {NULL,        NULL}
};
