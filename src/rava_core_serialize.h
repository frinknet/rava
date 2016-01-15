#include <stdint.h>
#include <stddef.h>

#define RAVA_CODEC_TREF 1
#define RAVA_CODEC_TVAL 2
#define RAVA_CODEC_TUSR 3

rava_buf_t* ravaL_buf_new(size_t size);
void ravaL_buf_close(rava_buf_t* buf);
void ravaL_buf_need(rava_buf_t* buf, size_t len);
void ravaL_buf_init(rava_buf_t* buf, uint8_t* data, size_t len);
void ravaL_buf_put(rava_buf_t* buf, uint8_t val);
void ravaL_buf_write(rava_buf_t* buf, uint8_t* data, size_t len);
void ravaL_buf_write_uleb128(rava_buf_t* buf, uint32_t val);
uint8_t ravaL_buf_get(rava_buf_t* buf);
uint8_t* ravaL_buf_read(rava_buf_t* buf, size_t len);
uint32_t ravaL_buf_read_uleb128(rava_buf_t* buf);
uint8_t ravaL_buf_peek(rava_buf_t* buf);
int ravaL_writer(lua_State* L, const char* str, size_t len, void* buf);
int ravaL_serialize_encode(lua_State* L, int narg);
int ravaL_serialize_decode(lua_State* L);
