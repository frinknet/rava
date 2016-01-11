#include "rava_stream.h"

void ray_close_cb(uv_handle_t* handle)
{
  ray_agent_t* self = container_of(handle, ray_agent_t, h);
  TRACE("closed: %p\n", self);
  ray_fiber_t* curr = (ray_fiber_t*)self->data;
  if (curr) ray_resume(curr, 0);
  if (self->ref != LUA_NOREF) {
    TRACE("UNREF: %i!\n", self->ref);
    luaL_unref(RAY_MAIN->L, LUA_REGISTRYINDEX, self->ref);
    self->ref = LUA_NOREF;
  }
}

int ray_close(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  if (!uv_is_closing(&self->h.handle)) {
    ray_fiber_t* curr = ray_current(L);
    self->data = curr;
    uv_close(&self->h.handle, ray_close_cb);
    return ray_suspend(curr);
  }
  return 0;
}

void ray_alloc_cb(uv_handle_t* h, size_t len, uv_buf_t* buf)
{
  ray_agent_t* self = container_of(h, ray_agent_t, h);
  if (len > RAY_BUFFER_SIZE) len = RAY_BUFFER_SIZE;
  ray_buf_need(&self->buf, len);
  *buf = uv_buf_init(self->buf.base + self->buf.offs, len);
}

void ray_close_null_cb(uv_handle_t* handle)
{
  TRACE("running null close cb - data: %p\n", handle->data);
}

void ray_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
  ray_agent_t* self = container_of(stream, ray_agent_t, h);

  uv_read_stop(stream);
  self->flags &= ~RAY_STARTED;

  TRACE("nread: %i\n", (int)nread);
  if (nread >= 0) {
    assert(!QUEUE_EMPTY(&self->fiber_queue));
    QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
    ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

    QUEUE_REMOVE(q);
    QUEUE_INIT(q);

    lua_pushlstring(f->L, (const char*)buf->base, nread);
    ray_buf_clear(&self->buf);
    ray_resume(f, 1);
  }
  else {
    while (!QUEUE_EMPTY(&self->fiber_queue)) {
      QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
      ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

      QUEUE_REMOVE(q);
      QUEUE_INIT(q);

      ray_resume(f, ray_push_error(f->L, nread));
    }
    if (!uv_is_closing((uv_handle_t*)stream)) {
      uv_close((uv_handle_t*)stream, ray_close_null_cb);
    }
  }
}

int ray_read(lua_State* L)
{
  TRACE("reading\n");
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  ray_fiber_t* curr = ray_current(L);

  assert((self->flags & RAY_STARTED) == 0);
  self->flags |= RAY_STARTED;
  uv_read_start(&self->h.stream, ray_alloc_cb, ray_read_cb);

  assert(QUEUE_EMPTY(&curr->queue));
  QUEUE_INSERT_TAIL(&self->fiber_queue, &curr->queue);
  return ray_suspend(curr);
}

void ray_write_cb(uv_write_t* req, int status)
{
  ray_fiber_t* curr = container_of(req, ray_fiber_t, r);
  TRACE("finished writing - curr: %p\n", curr);
  ray_resume(curr, 1);
}

int ray_write(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  ray_fiber_t* curr = ray_current(L);

  size_t len;
  const char* base = lua_tolstring(L, 2, &len);
  assert((self->flags & RAY_STARTED) == 0);
  uv_buf_t buf = uv_buf_init((char*)base, len);
  TRACE("writing - curr: %p\n", curr);
  uv_write(&curr->r.write, &self->h.stream, &buf, 1, ray_write_cb);

  lua_settop(L, 1);
  return ray_suspend(curr);
}

void ray_shutdown_cb(uv_shutdown_t* req, int status)
{
  ray_fiber_t* curr = container_of(req, ray_fiber_t, r);
  TRACE("shutdown: %p\n", curr);
  lua_settop(curr->L, 0);
  lua_pushboolean(curr->L, 1);
  ray_resume(curr, 1);
}

int ray_shutdown(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  ray_fiber_t* curr = ray_current(L);

  TRACE("shutdown: self: %p, curr: %p\n", self, curr);
  uv_shutdown_t* req = &curr->r.shutdown;

  int rc = uv_shutdown(req, &self->h.stream, ray_shutdown_cb);
  if (rc) return ray_push_error(L, rc);

  if (self->flags & RAY_STARTED) {
    self->flags &= ~RAY_STARTED;
    uv_read_stop(&self->h.stream);
  }

  return ray_suspend(curr);
}

void ray_listen_cb(uv_stream_t* server, int status)
{
  ray_agent_t* self = container_of(server, ray_agent_t, h);
  TRACE("agent: %p\n", self);
  if (!QUEUE_EMPTY(&self->fiber_queue)) {
    QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
    ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

    QUEUE_REMOVE(q);
    QUEUE_INIT(q);

    ray_agent_t* conn = (ray_agent_t*)lua_touserdata(f->L, 2);
    TRACE("new connection: %p\n", conn);

    int rc = uv_accept(server, &conn->h.stream);
    if (rc) {
      ray_resume(f, ray_push_error(f->L, rc));
      return;
    }
    ray_resume(f, 1);
  }
  else {
    TRACE("count++\n");
    self->count++;
  }
}

int ray_listen(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  int backlog = luaL_optint(L, 2, 128);
  self->count = 0;

  int rc = uv_listen(&self->h.stream, backlog, ray_listen_cb);
  if (rc) return ray_push_error(L, rc);

  return 1;
}

int ray_accept(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  ray_agent_t* conn = (ray_agent_t*)lua_touserdata(L, 2);
  TRACE("accepting - count: %i\n", self->count);
  if (self->count > 0) {
    self->count--;

    int rc = uv_accept(&self->h.stream, &conn->h.stream);
    if (rc) return ray_push_error(L, rc);

    return 1;
  }
  else {
    ray_fiber_t* curr = ray_current(L);
    assert(curr != RAY_MAIN);
    assert(QUEUE_EMPTY(&curr->queue));
    QUEUE_INSERT_TAIL(&self->fiber_queue, &curr->queue);
    return ray_suspend(curr);
  }
}

void ray_connect_cb(uv_connect_t* req, int status)
{
  ray_fiber_t* curr = container_of(req, ray_fiber_t, r);
  if (status) {
    ray_resume(curr, ray_push_error(curr->L, status));
  }
  else {
    lua_pushboolean(curr->L, 1);
    ray_resume(curr, 1);
  }
}
