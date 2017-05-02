#include <stdio.h>
#include <pthread.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "luajit.h"
#include "rava.h"

#include "lj_arch.h"

#if LJ_TARGET_POSIX
#include <unistd.h>
#define lua_stdin_is_tty()	isatty(0)
#elif LJ_TARGET_WINDOWS
#include <io.h>
#ifdef __BORLANDC__
#define lua_stdin_is_tty()	isatty(_fileno(stdin))
#else
#define lua_stdin_is_tty()	_isatty(_fileno(stdin))
#endif
#else
#define lua_stdin_is_tty()	1
#endif

#if !LJ_TARGET_CONSOLE
#include <signal.h>
#endif

LUA_API int luaopen_rava_fs(lua_State *L);
LUA_API int luaopen_rava_process(lua_State *L);
LUA_API int luaopen_rava_socket(lua_State *L);
LUA_API int luaopen_rava_system(lua_State *L);

static lua_State *globalL = NULL;
static const char *progname = "rava";

/**
 * Create a lua container from and run the lua code you pass to it
 *
 * returns new lua state
 */
static lua_State* rava_newlua() {
	int i;

	lua_State* L = luaL_newstate();

	if (!L) {
		printf("Error: LuaJIT unavailable!!\n");

		return NULL;
	}

	// stop collector during initialization
	lua_gc(L, LUA_GCSTOP, 0);
	// open libraries
	luaL_openlibs(L);
	// restart collector
	lua_gc(L, LUA_GCRESTART, -1);

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");

	// Remove package
	lua_remove(L, -2);
	
	// Store rava module definition at packages.preload.rava
	lua_pushcfunction(L, luaopen_rava);
	lua_setfield(L, -2, "rava");

	lua_pushcfunction(L, luaopen_rava_fs);
	lua_setfield(L, -2, "rava.fs");

	lua_pushcfunction(L, luaopen_rava_process);
	lua_setfield(L, -2, "rava.process");

	lua_pushcfunction(L, luaopen_rava_socket);
	lua_setfield(L, -2, "rava.socket");

	lua_pushcfunction(L, luaopen_rava_system);
	lua_setfield(L, -2, "rava.system");

	lua_pop(L, 1);

	return L;
}

static void lstop(lua_State *L, lua_Debug *ar)
{
  (void)ar;  /* unused arg. */
  lua_sethook(L, NULL, 0, 0);
  /* Avoid luaL_error -- a C hook doesn't add an extra frame. */
  luaL_where(L, 0);
  lua_pushfstring(L, "%sinterrupted!", lua_tostring(L, -1));
  lua_error(L);
}

static void laction(int i)
{
  signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
			 terminate process (default action) */
  lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}
static void l_message(const char *pname, const char *msg)
{
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
  fflush(stderr);
}

static int report(lua_State *L, int status)
{
  if (status && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message(progname, msg);
    lua_pop(L, 1);
  }
  return status;
}

static int traceback(lua_State *L)
{
  if (!lua_isstring(L, 1)) { /* Non-string error object? Try metamethod. */
    if (lua_isnoneornil(L, 1) ||
	!luaL_callmeta(L, 1, "__tostring") ||
	!lua_isstring(L, -1))
      return 1;  /* Return non-string error object. */
    lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
  }
  luaL_traceback(L, L, lua_tostring(L, 1), 1);
  return 1;
}

static int docall(lua_State *L, int narg, int clear)
{
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
#if !LJ_TARGET_CONSOLE
  signal(SIGINT, laction);
#endif
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
#if !LJ_TARGET_CONSOLE
  signal(SIGINT, SIG_DFL);
#endif
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}
static int dostring(lua_State *L, const char *s, const char *name)
{
  int r = luaL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
  return report(L, r);
}

static int dolibrary(lua_State *L, const char *name)
{
  lua_getglobal(L, "require");
  lua_pushstring(L, name);
  return report(L, docall(L, 1, 1));
}

/**
 * Main loop sets up the first lua contaner with arg
 *
 * argc count of arguments
 * argv array of argument values
 *
 * returns return code
 */
int main(int argc, char *argv[])
{
	int i;
	int r;

	lua_State* L = rava_newlua();

  globalL = L;

	lua_newtable(L);

  if (argv[0] && argv[0][0]) progname = argv[0];

	for (i = 0; i < argc; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, argv[i]);
		lua_settable(L, -3);
	}

	lua_setglobal(L, "arg");

	r = dolibrary(L, "init");
	r = dolibrary(L, "main");

	lua_close(L);

	return r? EXIT_FAILURE : EXIT_SUCCESS;
}
