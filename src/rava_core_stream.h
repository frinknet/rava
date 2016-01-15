void ravaL_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf);
void ravaL_connect_cb(uv_connect_t* req, int status);
void ravaL_stream_free(rava_object_t* self);
void ravaL_stream_close(rava_object_t* self);
int ravaL_stream_start(rava_object_t* self);
int ravaL_stream_stop(rava_object_t* self);

luaL_Reg rava_stream_meths[32];
