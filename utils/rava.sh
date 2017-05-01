#!/bin/bash

UTLDIR="$(dirname $0)"
RAVDIR="$(cd $UTLDIR/..; pwd)"
LIBDIR="$RAVDIR/libs"
LUADIR="$RAVDIR/lua"
LUAJIT="$LIBDIR/luajit/luajit"

export LUA_PATH="./?.lua;./?.so;$LUADIR/?.lua;$RAVDIR/?.lua;$RAVDIR/?.so"
export RAVABIN=$RAVDIR/utils/$(basename $0)

ARGS=()

for x in "$@"; do
	 if [[ $x == *"*"* ]]; then
			ARGS+=( $(cd $LUADIR && ls $x) )
	 else
			ARGS+=( $x )
	 fi
done

$LUAJIT $LUADIR/main.lua "${ARGS[@]}"
