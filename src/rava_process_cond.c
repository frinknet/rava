#include "rava.h"
#include "rava_common.h"

int rava_new_cond(lua_State* L)
{
  rava_cond_t* cond = (rava_cond_t*)lua_newuserdata(L, sizeof(rava_cond_t));

  lua_pushvalue(L, 1);
  luaL_getmetatable(L, RAVA_PROCESS_COND);
  lua_setmetatable(L, -2);
  ravaL_cond_init(cond);

  return 1;
}

int ravaL_cond_init(rava_cond_t* cond)
{
  QUEUE_INIT(cond);

  return 1;
}

int ravaL_cond_wait(rava_cond_t* cond, rava_state_t* curr)
{
  QUEUE_INSERT_TAIL(cond, &curr->cond);
  TRACE("SUSPEND state %p\n", curr);

  return ravaL_state_suspend(curr);
}

int ravaL_cond_signal(rava_cond_t* cond)
{
  QUEUE* q;
  rava_state_t* s;

  if (!QUEUE_EMPTY(cond)) {
    q = QUEUE_HEAD(cond);
    s = QUEUE_DATA(q, rava_state_t, cond);

    QUEUE_REMOVE(q);
    TRACE("READY state %p\n", s);
    ravaL_state_ready(s);

    return 1;
  }

  return 0;
}

int ravaL_cond_broadcast(rava_cond_t* cond)
{
  QUEUE* q;
  rava_state_t* s;

  int roused = 0;

  while (!QUEUE_EMPTY(cond)) {
    q = QUEUE_HEAD(cond);
    s = QUEUE_DATA(q, rava_state_t, cond);

    QUEUE_REMOVE(q);
    TRACE("READY state %p\n", s);
    ravaL_state_ready(s);

    ++roused;
  }

  return roused;
}

static int rava_cond_wait(lua_State *L)
{
  rava_cond_t*  cond  = (rava_cond_t*)lua_touserdata(L, 1);
  rava_state_t* curr;

  if (!lua_isnoneornil(L, 2)) {
    curr = (rava_state_t*)luaL_checkudata(L, 2, RAVA_PROCESS_FIBER);
	} else {
    curr = (rava_state_t*)ravaL_state_self(L);
  }

  ravaL_cond_wait(cond, curr);

  return 1;
}

static int rava_cond_signal(lua_State *L)
{
  rava_cond_t* cond = (rava_cond_t*)lua_touserdata(L, 1);
  ravaL_cond_signal(cond);

  return 1;
}

static int rava_cond_broadcast(lua_State *L)
{
  rava_cond_t* cond = (rava_cond_t*)lua_touserdata(L, 1);
  ravaL_cond_broadcast(cond);

  return 1;
}

static int rava_cond_free(lua_State *L)
{
  rava_cond_t* cond = (rava_cond_t*)lua_touserdata(L, 1);
  (void)cond;

  return 0;
}

static int rava_cond_tostring(lua_State *L)
{
  rava_cond_t* cond = (rava_cond_t*)luaL_checkudata(L, 1, RAVA_PROCESS_COND);
  lua_pushfstring(L, "userdata<%s>: %p", RAVA_PROCESS_COND, cond);

  return 1;
}

luaL_Reg rava_cond_meths[] = {
  {"wait",      rava_cond_wait},
  {"signal",    rava_cond_signal},
  {"broadcast", rava_cond_broadcast},
  {"__gc",      rava_cond_free},
  {"__tostring",rava_cond_tostring},
  {NULL,        NULL}
};

LUA_API int luaopen_rava_process_cond(lua_State* L)
{
  ravaL_class(L, RAVA_PROCESS_COND, rava_cond_meths);
  lua_pop(L, 1);
	lua_pushcfunction(L, rava_new_cond);

  return 1;
}
