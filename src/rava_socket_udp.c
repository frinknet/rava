#include "rava.h"
#include <string.h>

static int rava_new_udp(lua_State* L)
{
  rava_state_t*  curr = ravaL_state_self(L);
  rava_object_t* self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));
  luaL_getmetatable(L, RAVA_NET_UDP_T);
  lua_setmetatable(L, -2);
  ravaL_object_init(curr, self);

  uv_udp_init(ravaL_event_loop(L), &self->h.udp);
  return 1;
}

static int rava_udp_bind(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_NET_UDP_T);
  const char*   host = luaL_checkstring(L, 2);
  int           port = luaL_checkint(L, 3);

  int flags = 0;

  struct sockaddr_in addr;

	uv_ip4_addr(host, port, &addr);

	int r = uv_udp_bind(&self->h.udp, (const struct sockaddr*)&addr, flags);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  return 0;
}

static void _send_cb(uv_udp_send_t* req, int status)
{
  rava_state_t* curr = container_of(req, rava_state_t, req);
  ravaL_state_ready(curr);
}

static int rava_udp_send(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_NET_UDP_T);
  rava_state_t*  curr = ravaL_state_self(L);

  size_t len;

  const char* host = luaL_checkstring(L, 2);
  int         port = luaL_checkint(L, 3);
  const char* mesg = luaL_checklstring(L, 4, &len);

  uv_buf_t buf = uv_buf_init((char*)mesg, len);

  struct sockaddr_in addr;

	uv_ip4_addr(host, port, &addr);

	int r = uv_udp_send(&curr->req.udp_send, &self->h.udp, &buf, 1,
		(const struct sockaddr *)&addr, _send_cb);

  if (r < 0) {
    /* TODO: this shouldn't be fatal */
    return luaL_error(L, uv_strerror(r));
  }

  return ravaL_state_suspend(curr);
}

static void _recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
	const struct sockaddr* addr, unsigned flags)
{
  rava_object_t* self = container_of(handle, rava_object_t, h);
  QUEUE* q;
  rava_state_t* s;

  char host[INET6_ADDRSTRLEN];
  int  port = 0;

  QUEUE_FOREACH(q, &self->rouse) {
    s = QUEUE_DATA(q, rava_state_t, cond);

    lua_settop(s->L, 0);
    lua_pushlstring(s->L, buf->base, buf->len);

    if (addr->sa_family == PF_INET) {
      struct sockaddr_in* addr = (struct sockaddr_in*)addr;

      uv_ip4_name(addr, host, INET6_ADDRSTRLEN);

      port = addr->sin_port;
    } else if (addr->sa_family == PF_INET6) {
      struct sockaddr_in6* addr = (struct sockaddr_in6*)addr;

      uv_ip6_name(addr, host, INET6_ADDRSTRLEN);
      port = addr->sin6_port;
    }

    lua_pushstring(s->L, host);
    lua_pushinteger(s->L, port);
    /* [ mesg, host, port ] */
  }

  ravaL_cond_signal(&self->rouse);
}

static int rava_udp_recv(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_NET_UDP_T);
  if (!ravaL_object_is_started(self)) {
    self->flags |= RAVA_OSTARTED;

    uv_udp_recv_start(&self->h.udp, ravaL_alloc_cb, _recv_cb);
  }
  return ravaL_cond_wait(&self->rouse, ravaL_state_self(L));
}

static const char* RAVA_UDP_MEMBERSHIP_OPTS[] = { "join", "leave", NULL };

int rava_udp_membership(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_NET_UDP_T);
  const char*  iaddr = luaL_checkstring(L, 3);
  const char*  maddr = luaL_checkstring(L, 2);

  int option = luaL_checkoption(L, 4, NULL, RAVA_UDP_MEMBERSHIP_OPTS);
  uv_membership membership = option ? UV_LEAVE_GROUP : UV_JOIN_GROUP;

	int r = uv_udp_set_membership(&self->h.udp, maddr, iaddr, membership);

  if (r < 0) {
    return luaL_error(L, uv_strerror(r));
  }

  return 0;
}

static int rava_udp_free(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  ravaL_object_close(self);

  return 1;
}

static int rava_udp_tostring(lua_State *L)
{
  rava_object_t *self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_NET_UDP_T);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_NET_UDP_T, self);

  return 1;
}

luaL_Reg rava_udp_meths[] = {
  {"bind",       rava_udp_bind},
  {"send",       rava_udp_send},
  {"recv",       rava_udp_recv},
  {"membership", rava_udp_membership},
  {"__gc",       rava_udp_free},
  {"__tostring", rava_udp_tostring},
  {NULL,         NULL}
};
