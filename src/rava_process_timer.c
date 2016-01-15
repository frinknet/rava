#include "rava.h"
#include "rava_core.h"
#include "rava_process.h"

int rava_new_timer(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)lua_newuserdata(L, sizeof(rava_object_t));
  luaL_getmetatable(L, RAVA_PROCESS_TIMER);
  lua_setmetatable(L, -2);

  rava_state_t* curr = ravaL_state_self(L);
  uv_timer_init(ravaL_event_loop(L), &self->h.timer);
  ravaL_object_init(curr, self);

  return 1;
}

static void _timer_cb(uv_timer_t* handle)
{
  rava_object_t* self = container_of(handle, rava_object_t, h);
  QUEUE* q;
  rava_state_t* s;

  QUEUE_FOREACH(q, &self->rouse) {
    s = QUEUE_DATA(q, rava_state_t, cond);
    TRACE("rouse %p\n", s);
    lua_settop(s->L, 0);
  }

  ravaL_cond_broadcast(&self->rouse);
}

/* methods */
static int rava_timer_start(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_TIMER);
  int64_t timeout = luaL_optlong(L, 2, 0L);
  int64_t repeat  = luaL_optlong(L, 3, 0L);
  int rv = uv_timer_start(&self->h.timer, _timer_cb, timeout, repeat);
  lua_pushinteger(L, rv);
  return 1;
}

static int rava_timer_again(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_TIMER);
  lua_pushinteger(L, uv_timer_again(&self->h.timer));
  return 1;
}

static int rava_timer_stop(lua_State* L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_TIMER);
  lua_pushinteger(L, uv_timer_stop(&self->h.timer));
  return 1;
}

static int rava_timer_wait(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_TIMER);
  rava_state_t* state = ravaL_state_self(L);
  return ravaL_cond_wait(&self->rouse, state);
}

static int rava_timer_free(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)lua_touserdata(L, 1);
  ravaL_object_close(self);
  return 1;
}

static int rava_timer_tostring(lua_State *L)
{
  rava_object_t* self = (rava_object_t*)luaL_checkudata(L, 1, RAVA_PROCESS_TIMER);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_PROCESS_TIMER, self);
  return 1;
}

luaL_Reg rava_timer_meths[] = {
  {"start",     rava_timer_start},
  {"again",     rava_timer_again},
  {"stop",      rava_timer_stop},
  {"wait",      rava_timer_wait},
  {"__gc",      rava_timer_free},
  {"__tostring",rava_timer_tostring},
  {NULL,        NULL}
};

LUA_API int luaopen_rava_process_timer(lua_State* L)
{
  ravaL_class(L, RAVA_PROCESS_TIMER, rava_timer_meths);
  lua_pop(L, 1);
	lua_pushcfunction(L, rava_new_timer);

  return 1;
}
