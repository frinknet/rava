#include "rava_buffer.h"

void ray_buf_need(ray_buf_t* buf, size_t len)
{
  size_t size = buf->size;
  size_t need = buf->offs + len;

  if (need > buf->size) {
    while (size < need) size *= 2;

    buf->base = (char*)realloc(buf->base, size);
    buf->size = size;
  }
}

void ray_buf_init(ray_buf_t* buf)
{
  buf->base = calloc(1, RAY_BUFFER_SIZE);
  buf->size = RAY_BUFFER_SIZE;
  buf->offs = 0;
}

void ray_buf_write(ray_buf_t* buf, char* data, size_t len)
{
  ray_buf_need(buf, len);
  memcpy(buf->base + buf->offs, data, len);

  buf->offs += len;
}

const char* ray_buf_read(ray_buf_t* buf, size_t* len)
{
  *len = buf->offs;
  buf->offs = 0;

  return buf->base;
}

void ray_buf_clear(ray_buf_t* buf)
{
  memset(buf->base, 0, buf->size);

  buf->offs = 0;
}
