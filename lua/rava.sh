LUADIR="$(dirname $0)"
RAVDIR="$LUADIR/.."
LIBDIR="$RAVDIR/libs"
LUAJIT="$LIBDIR/luajit/luajit"

export LUA_PATH="./?.lua;$LUADIR/?.lua;$RAVDIR/?.lua;$RAVDIR/?.so"
export RAVABIN=$0

$LUAJIT $LUADIR/main.lua $@
