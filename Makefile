VERSION=2.0.1

export PREFIX= /usr/local

LUA_DEPS=deps/luajit/src/luajit deps/luajit/src/lua.h deps/luajit/src/lualib.h \
deps/luajit/src/luaconf.h deps/luajit/src/lauxlib.h deps/luajit/src/libluajit.a \
deps/luajit/src/jit/bcsave.lua
LUA_LIBS=libs/luajit libs/lua.h libs/lualib.h libs/luaconf.h libs/lauxlib.h \
libs/libluajit.a deps/luajit/src/jit/bcsave.lua

DPREFIX= $(DESTDIR)$(PREFIX)
INSTALL_BIN=   $(DPREFIX)/bin
INSTALL_DEP=   rava

default all : libs/luajit src/rava.lua modules/*.lua
	$(MAKE) git
	@echo "==== Building Rava $(VERSION) ===="
	CCARGS="$(CCARGS)" libs/luajit src/main.lua -csmr --compile=rava src/main.lua
	@echo "==== Successfully built Rava $(VERSION) ===="

libs/rava.a src/rava.o: $(INSTALL_DEP) src/main.c
	@echo "==== Generating Rava Lib $(VERSION) to $(PREFIX) ===="
	$(CC) -c src/rava.c -Ilibs/ -o src/rava.o
	$(LD) -r src/rava.o libs/libluajit.a -o libs/rava.a
	@echo "==== Successfully generated Rava Lib $(VERSION) to $(PREFIX) ===="

install: $(INSTALL_DEP)
	@echo "==== Installing Rava $(VERSION) to $(PREFIX) ===="
	cp $+ $(INSTALL_BIN)
	@echo "==== Successfully installed Rava $(VERSION) to $(PREFIX) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm $(INSTALL_BIN)/rava
	@echo "==== Successfully uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

git:
	@echo "==== Update Git $(VERSION) ===="
	#git pull
	git submodule init
	git submodule update
	@echo "==== Successfully update git $(VERSION) ===="

clean:
	$(MAKE) clean -C deps/luajit/
	rm -r libs/* src/*.o modules/*.o rava 2>/dev/null

$(LUA_LIBS): $(LUA_DEPS)
	cp $+ libs/

$(LUA_DEPS):
	$(MAKE) -C deps/luajit/
