#include "rava_lib.h"

void ray_buf_need(ray_buf_t* buf, size_t len);
void ray_buf_init(ray_buf_t* buf);
void ray_buf_write(ray_buf_t* buf, char* data, size_t len);
const char* ray_buf_read(ray_buf_t* buf, size_t* len);
void ray_buf_clear(ray_buf_t* buf);
