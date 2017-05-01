UTLDIR="$(dirname $0)"
RAVDIR="$(cd $UTLDIR/..; pwd)"
LIBDIR="$RAVDIR/libs"
LUADIR="$RAVDIR/lua"
LUAJIT="$LIBDIR/luajit/luajit"

export LUA_PATH="./?.lua;$LUADIR/?.lua;$RAVDIR/?.lua;$RAVDIR/?.so"
export RAVABIN=$RAVDIR/utils/$(basename $0)

cd $LUADIR && $LUAJIT main.lua $@
