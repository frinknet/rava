#include "rava_agent.h"
#include "rava_buffer.h"

ray_agent_t* ray_agent_new(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_newuserdata(L, sizeof(ray_agent_t));

  self->flags = 0;
  self->count = 0;
  self->data  = NULL;

  ray_buf_init(&self->buf);
  QUEUE_INIT(&self->fiber_queue);

  lua_pushvalue(L, 1);
  self->ref = luaL_ref(L, LUA_REGISTRYINDEX);

  return self;
}

int ray_agent_free(lua_State* L)
{
  ray_agent_t* self = (ray_agent_t*)lua_touserdata(L, 1);
  TRACE("FREE: %p\n", self);
  if (self->buf.base) {
    free(self->buf.base);
    self->buf.base = NULL;
    self->buf.size = 0;
    self->buf.offs = 0;
  }
  return 0;
}
