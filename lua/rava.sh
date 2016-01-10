DIR="$(dirname $0)"
LUAJIT="$DIR/../libs/luajit/luajit"

LUA_PATH="$DIR/?.lua;?;?.lua;?.so" $LUAJIT $DIR/main.lua $@
