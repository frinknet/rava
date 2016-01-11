#include "rava_lib.h"

void ray_close_cb(uv_handle_t* handle);
int ray_close(lua_State* L);
void ray_alloc_cb(uv_handle_t* h, size_t len, uv_buf_t* buf);
void ray_close_null_cb(uv_handle_t* handle);
void ray_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
int ray_read(lua_State* L);
void ray_write_cb(uv_write_t* req, int status);
int ray_write(lua_State* L);
void ray_shutdown_cb(uv_shutdown_t* req, int status);
int ray_shutdown(lua_State* L);
void ray_listen_cb(uv_stream_t* server, int status);
int ray_listen(lua_State* L);
int ray_accept(lua_State* L);
void ray_connect_cb(uv_connect_t* req, int status);
