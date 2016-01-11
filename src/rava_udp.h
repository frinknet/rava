#include "rava_lib.h"

int ray_udp_new(lua_State* L);
int ray_udp_bind(lua_State* L);
void ray_udp_send_cb(uv_udp_send_t* req, int status);
int ray_udp_send(lua_State* L);
void ray_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t *buf,
  const struct sockaddr* peer, unsigned flags);
void ray_udp_alloc_cb(uv_handle_t* handle, size_t len, uv_buf_t* buf);
int ray_udp_recv(lua_State* L);
int ray_udp_membership(lua_State* L);
