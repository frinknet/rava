#include "rava.h"

#include <stdint.h>
#include <stddef.h>

/* TODO: make this buffer stuff generic */
typedef struct rava_buf_t {
  size_t   size;
  uint8_t* head;
  uint8_t* base;
} rava_buf_t;

static int encode_table(lua_State* L, rava_buf_t *buf, int seen);
static int decode_table(lua_State* L, rava_buf_t* buf, int seen);

rava_buf_t* ravaL_buf_new(size_t size)
{
  if (!size) size = 128;

  rava_buf_t* buf = (rava_buf_t*)malloc(sizeof(rava_buf_t));

  buf->base = (uint8_t*)malloc(size);
  buf->size = size;
  buf->head = buf->base;

  return buf;
}

void ravaL_buf_close(rava_buf_t* buf)
{
  free(buf->base);

  buf->head = NULL;
  buf->base = NULL;
  buf->size = 0;
}

void ravaL_buf_need(rava_buf_t* buf, size_t len)
{
  size_t size = buf->size;
  if (!size) {
    size = 128;
    buf->base = (uint8_t*)malloc(size);
    buf->size = size;
    buf->head = buf->base;
  }
  ptrdiff_t head = buf->head - buf->base;
  ptrdiff_t need = head + len;
  while (size < need) size *= 2;
  if (size > buf->size) {
    buf->base = (uint8_t*)realloc(buf->base, size);
    buf->size = size;
    buf->head = buf->base + head;
  }
}

void ravaL_buf_init(rava_buf_t* buf, uint8_t* data, size_t len)
{
  ravaL_buf_need(buf, len);
  memcpy(buf->base, data, len);
  buf->head += len;
}

void ravaL_buf_put(rava_buf_t* buf, uint8_t val)
{
  ravaL_buf_need(buf, 1);
  *(buf->head++) = val;
}

void ravaL_buf_write(rava_buf_t* buf, uint8_t* data, size_t len)
{
  ravaL_buf_need(buf, len);
  memcpy(buf->head, data, len);
  buf->head += len;
}

void ravaL_buf_write_uleb128(rava_buf_t* buf, uint32_t val)
{
  ravaL_buf_need(buf, 5);
  size_t   n = 0;
  uint8_t* p = buf->head;
  for (; val >= 0x80; val >>= 7) {
    p[n++] = (uint8_t)((val & 0x7f) | 0x80);
  }
  p[n++] = (uint8_t)val;
  buf->head += n;
}

uint8_t ravaL_buf_get(rava_buf_t* buf)
{
  return *(buf->head++);
}

uint8_t* ravaL_buf_read(rava_buf_t* buf, size_t len)
{
  uint8_t* p = buf->head;
  buf->head += len;
  return p;
}

uint32_t ravaL_buf_read_uleb128(rava_buf_t* buf)
{
  const uint8_t* p = (const uint8_t*)buf->head;
  uint32_t v = *p++;
  if (v >= 0x80) {
    int sh = 0;
    v &= 0x7f;
    do {
     v |= ((*p & 0x7f) << (sh += 7));
    } while (*p++ >= 0x80);
  }
  buf->head = (uint8_t*)p;
  return v;
}

uint8_t ravaL_buf_peek(rava_buf_t* buf)
{
  return *buf->head;
}
