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

CCARGS=-Ilibs/

CC=gcc
LD=ld
RAVA=CCARGS="$(CCARGS)" libs/luajit src/main.lua

default all $(INSTALL_DEP): libs/luajit libs/ravastore.o src/rava.lua modules/*.lua
	$(MAKE) git
	@echo "==== Building Rava $(VERSION) ===="
	$(RAVA) -csmr --compile=rava src/main.lua
	@echo "==== Successfully built Rava $(VERSION) ===="

libs/ravastore.o libs/ravastore.lua: libs/luajit libs/rava.a
	@echo "==== Generating Rava Store $(VERSION) to $(PREFIX) ===="
	$(RAVA) --store=ravastore libs/ravastore.o libs/rava.a
	@echo "==== Successfully generated Rava Store $(VERSION) to $(PREFIX) ===="

libs/rava.a src/rava.o: src/rava.c
	@echo "==== Generating Rava Kernel $(VERSION) to $(PREFIX) ===="
	$(RAVA) --generate=init src/init.lua src/init.lua.o
	$(CC) -c src/rava.c -Ilibs/ -o src/rava.o
	$(LD) -r src/rava.o src/init.lua.o libs/libluajit.a -o libs/rava.a
	@echo "==== Successfully generated Rava Kernel $(VERSION) to $(PREFIX) ===="

install: $(INSTALL_DEP)
	@echo "==== Installing Rava $(VERSION) to $(PREFIX) ===="
	cp $+ $(INSTALL_BIN)
	@echo "==== Successfully installed Rava $(VERSION) to $(PREFIX) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm -f $(INSTALL_BIN)/rava
	@echo "==== Successfully uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

git:
	@echo "==== Update Git $(VERSION) ===="
	#git pull
	git submodule init
	git submodule update
	@echo "==== Successfully update git $(VERSION) ===="

clean:
	$(MAKE) clean -C deps/luajit/
	rm -rf libs/* src/*.o modules/*.o rava

$(LUA_LIBS): $(LUA_DEPS)
	cp $+ libs/

$(LUA_DEPS):
	$(MAKE) -C deps/luajit/
