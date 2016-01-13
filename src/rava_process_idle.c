#include "rava.h"

static void _idle_cb(uv_idle_t* handle)
{
  rava_object_t* self = container_of(handle, rava_object_t, h);
  QUEUE* q;
  rava_state_t* s;

  QUEUE_FOREACH(q, &self->rouse) {
    s = QUEUE_DATA(q, rava_state_t, cond);
  }

  ravaL_cond_broadcast(&self->rouse);
}

static int rava_new_idle(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));
  luaL_getmetatable(L, RAVA_IDLE_T);
  lua_setmetatable(L, -2);

  rava_state_t* curr = ravaL_state_self(L);
  uv_idle_init(ravaL_event_loop(L), &self->h.idle);
  ravaL_object_init(curr, self);

  return 1;
}

/* methods */
static int rava_idle_start(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_IDLE_T);
  int r = uv_idle_start(&self->h.idle, _idle_cb);

  lua_pushinteger(L, r);

  return 1;
}

static int rava_idle_stop(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_IDLE_T);
  lua_pushinteger(L, uv_idle_stop(&self->h.idle));
  return 1;
}

static int rava_idle_wait(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_IDLE_T);
  rava_state_t* state = ravaL_state_self(L);
  return ravaL_cond_wait(&self->rouse, state);
}

static int rava_idle_free(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  ravaL_object_close(self);
  return 1;
}

static int rava_idle_tostring(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_IDLE_T);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_IDLE_T, self);
  return 1;
}

luaL_Reg rava_idle_funcs[] = {
  {"create",    rava_new_idle},
  {NULL,        NULL}
};

luaL_Reg rava_idle_meths[] = {
  {"start",     rava_idle_start},
  {"stop",      rava_idle_stop},
  {"wait",      rava_idle_wait},
  {"__gc",      rava_idle_free},
  {"__tostring",rava_idle_tostring},
  {NULL,        NULL}
};
