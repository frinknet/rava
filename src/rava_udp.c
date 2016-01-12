#include "rava_udp.h"
#include "rava_agent.h"
#include "rava_stream.h"

int ray_udp_new(lua_State* L)
{
  ray_agent_t* self = ray_agent_new(L);
  luaL_getmetatable(L, RAY_CLASS_UDP);
  lua_setmetatable(L, -2);
  uv_udp_init(uv_default_loop(), &self->h.udp);
  return 1;
}

int ray_udp_bind(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, RAY_CLASS_UDP);
  const char*  host = luaL_checkstring(L, 2);
  int          port = luaL_checkint(L, 3);

  int flags = 0;
  struct sockaddr_in addr;
  uv_ip4_addr(host, port, &addr);

  int rc = uv_udp_bind(&self->h.udp, (struct sockaddr*)&addr, flags);

  if (rc) return ray_push_error(L, rc);

  lua_settop(L, 1);

  return 1;
}

void ray_udp_send_cb(uv_udp_send_t* req, int status)
{
  ray_fiber_t* curr = container_of(req, ray_fiber_t, r);
  if (status < 0) {
    ray_resume(curr, ray_push_error(curr->L, status));
  } else {
    ray_resume(curr, 0);
  }
}

int ray_udp_send(lua_State* L) {
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, RAY_CLASS_UDP);
  ray_fiber_t* curr = ray_current(L);

  size_t len;

  const char* host = luaL_checkstring(L, 2);
  int         port = luaL_checkint(L, 3);
  const char* mesg = luaL_checklstring(L, 4, &len);

  uv_buf_t buf = uv_buf_init((char*)mesg, len);
  struct sockaddr_in addr;
  uv_ip4_addr(host, port, &addr);

  int rc = uv_udp_send(&curr->r.udp_send, &self->h.udp, &buf, 1, (struct sockaddr*)&addr, ray_udp_send_cb);

  if (rc) return ray_push_error(L, rc);

  return ray_suspend(curr);
}

void ray_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t *buf,
  const struct sockaddr* peer, unsigned flags)
{
  ray_agent_t* self = container_of(handle, ray_agent_t, h);
  if (nread == 0) return;

  char host[INET6_ADDRSTRLEN];
  int  port = 0;

  if (nread < 0) {
    if (self->flags & RAY_STARTED) {
      self->flags &= ~RAY_STARTED;
      uv_udp_recv_stop(&self->h.udp);
    }

    while (!QUEUE_EMPTY(&self->fiber_queue)) {
      QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
      ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

      QUEUE_REMOVE(q);
      QUEUE_INIT(q);

      ray_resume(f, ray_push_error(f->L, nread));
    }
    return;
  }

  if (peer->sa_family == PF_INET) {
    struct sockaddr_in* addr = (struct sockaddr_in*)peer;
    uv_ip4_name(addr, host, INET6_ADDRSTRLEN);
    port = addr->sin_port;
  } else if (peer->sa_family == PF_INET6) {
    struct sockaddr_in6* addr = (struct sockaddr_in6*)peer;
    uv_ip6_name(addr, host, INET6_ADDRSTRLEN);
    port = addr->sin6_port;
  }

  if (!QUEUE_EMPTY(&self->fiber_queue)) {
    QUEUE* q = QUEUE_HEAD(&self->fiber_queue);
    ray_fiber_t* f = QUEUE_DATA(q, ray_fiber_t, queue);

    QUEUE_REMOVE(q);
    QUEUE_INIT(q);

    lua_checkstack(f->L, 3);
    lua_pushlstring(f->L, (char*)buf->base, nread);

    lua_pushstring(f->L, host);
    lua_pushinteger(f->L, port);

    ray_buf_clear(&self->buf);
    ray_resume(f, 3);
  } else {
    memcpy(self->buf.base, (char*)(void*)&nread, sizeof(nread));
    ray_buf_write(&self->buf, host, strlen(host) + 1);
    ray_buf_write(&self->buf, (char*)(void*)&port, sizeof(port));
    self->count++;
  }
}

void ray_udp_alloc_cb(uv_handle_t* handle, size_t len, uv_buf_t* buf)
{
  ray_agent_t* self = container_of(handle, ray_agent_t, h);
  ray_buf_need(&self->buf, sizeof(ssize_t) + len);
  self->buf.offs += sizeof(ssize_t);
  *buf = uv_buf_init(self->buf.base + sizeof(ssize_t), len);
}

int ray_udp_recv(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, RAY_CLASS_UDP);
  ray_fiber_t* curr = ray_current(L);

  if (!(self->flags & RAY_STARTED)) {
    int rc = uv_udp_recv_start(&self->h.udp, ray_udp_alloc_cb, ray_udp_recv_cb);
    if (rc) return ray_push_error(L, rc);
    self->flags |= RAY_STARTED;
  }

  if (self->count > 0) {
    ssize_t slen;
    int     port;
    size_t  ulen;
    const char* mesg = ray_buf_read(&self->buf, &ulen);

    memcpy(&slen, mesg, sizeof(ssize_t));
    mesg += sizeof(ssize_t);

    lua_pushlstring(L, mesg, slen);
    mesg += slen;

    lua_pushstring(L, mesg);
    mesg += strlen(mesg) + 1;

    memcpy(&port, mesg, sizeof(int));
    lua_pushinteger(L, port);

    ray_buf_clear(&self->buf);

    self->count--;
    return 3;
  }

  assert(QUEUE_EMPTY(&curr->queue));
  QUEUE_INSERT_TAIL(&self->fiber_queue, &curr->queue);
  return ray_suspend(curr);
}

static const char* RAY_UDP_OPTS[] = { "join", "leave", NULL };

int ray_udp_membership(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)luaL_checkudata(L, 1, RAY_CLASS_UDP);
  const char*  iaddr = luaL_checkstring(L, 3);
  const char*  maddr = luaL_checkstring(L, 2);

  int option = luaL_checkoption(L, 4, NULL, RAY_UDP_OPTS);
  uv_membership membership = option ? UV_LEAVE_GROUP : UV_JOIN_GROUP;

  int rc = uv_udp_set_membership(&self->h.udp, maddr, iaddr, membership);

  if (rc) return ray_push_error(L, rc);

  return 1;
}

static luaL_Reg ray_udp_funcs[] = {
  {"listen",    ray_udp_new},
  {NULL,        NULL}
};

static luaL_Reg ray_udp_meths[] = {
  {"send",      ray_udp_send},
  {"recv",      ray_udp_recv},
  {"bind",      ray_udp_bind},
  {"membership",ray_udp_membership},
  {"__gc",      ray_agent_free},
  {NULL,        NULL}
};

LUA_API int LUA_MODULE(RAY_MODULE_UDP, lua_State* L)
{
  rayL_module(L, RAY_MODULE_UDP, ray_udp_funcs);
  rayL_class(L, RAY_CLASS_UDP, ray_udp_meths);
  lua_pop(L, 1);

  ray_init_main(L);

  return 1;
}
