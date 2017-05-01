VERSION=2.0.5

export PREFIX=/usr/local

DIR_DEPS=deps
DIR_DEPS_LUA=$(DIR_DEPS)/luajit/src
DIR_DEPS_LUV=$(DIR_DEPS)/libuv

DIR_LIBS=libs
DIR_LIBS_LUA=$(DIR_LIBS)/luajit
DIR_LIBS_LUV=$(DIR_LIBS)/libuv

DIR_LUA=lua

LUA_DEPS=\
	$(DIR_DEPS_LUA)/luajit \
	$(DIR_DEPS_LUA)/*.h \
	$(DIR_DEPS_LUA)/libluajit.a \
	$(DIR_DEPS_LUA)/jit/bcsave.lua

LUA_LIBS=\
	$(DIR_LIBS_LUA)/luajit \
	$(DIR_LIBS_LUA)/lua.h \
	$(DIR_LIBS_LUA)/lualib.h \
	$(DIR_LIBS_LUA)/luaconf.h \
	$(DIR_LIBS_LUA)/lauxlib.h \
	$(DIR_LIBS_LUA)/libluajit.a \
	$(DIR_LIBS_LUA)/bcsave.lua

LUV_DEPS=\
	$(DIR_DEPS_LUV)/.libs/libuv.a \
	$(DIR_DEPS_LUV)/src/queue.h \
	$(DIR_DEPS_LUV)/include/*.h

LUV_LIBS=\
	$(DIR_LIBS_LUV)/libuv.a \
	$(DIR_LIBS_LUV)/uv.h \
	$(DIR_LIBS_LUV)/queue.h

RAVA_DEPS=\
	src/librava.a \
	src/rava.so \
	src/ravamain.a

RAVA_LIBS=\
	libs/ravamain.a \
	libs/librava.a \
	libs/rava.so

RAVA_OBJS=\
	lua/rava.a \
	libs/ravastore.o

DPREFIX=$(DESTDIR)$(PREFIX)
INSTALL_BIN=$(DPREFIX)/bin
INSTALL_DEP=rava

CCARGS=-Ilibs/luajit/ -Ilibs/libuv/ -lpthread

CC:=gcc
LINK:=ld
COPY:=cp
REMOVE:=rm -rf
MKDIR:=mkdir -p
MAKING:=$(MAKE) -C
CLEAN:=$(MAKE) clean -C

EACH=utils/each.sh
RAVA=utils/rava.sh

all: rava rava.so

rava.so: deps
	@$(EACH) COPY $@
	@$(EACH) TO rava.so
	@$(COPY) src/rava.so .

rava: deps
	@$(EACH) RAVA \
		main.lua \
		config.lua \
		gen/bcsave.lua \
		modules/*.lua
	@$(EACH) BINARY rava
	@$(RAVA) -q -csn --binary=../rava \
		main.lua \
		config.lua \
		gen/bcsave.lua \
		modules/*.lua

install: $(INSTALL_DEP)
	@$(EACH) INSTALL $(INSTALL_BIN)/rava
	@sudo $(COPY) $(INSTALL_DEP) $(INSTALL_BIN)

uninstall:
	@$(EACH) REMOVE $(INSTALL_BIN)/rava
	@$(REMOVE) $(INSTALL_BIN)/rava

$(RAVA_DEPS): $(LUA_LIBS) $(LUV_LIBS)
	@$(EACH) MAKING src/
	@$(MAKING) src/

$(RAVA_LIBS): $(RAVA_DEPS)
	@$(EACH) COPY $@
	@$(COPY) $+ libs/

$(RAVA_OBJS): $(RAVA_LIBS)
	@$(EACH) RAVA lua/init.lua
	@$(EACH) BYTECODE lua/init.lua.o
	@$(RAVA) -q --bytecode=init init.lua init.lua.o
	@$(EACH) RAVA \
		init.lua.o \
		opt.lua \
		msg.lua \
		../libs/ravamain.a
	@$(EACH) LIBRARY lua/rava.a
	@$(RAVA) -q --library=rava \
		init.lua.o \
		opt.lua \
		msg.lua \
		../libs/ravamain.a
	@$(EACH) RAVA \
		 ../libs/ravastore.o \
		 rava.a
	@$(EACH) FILESTORE libs/ravastore.o
	@$(RAVA) -q --filestore=ravastore \
		 ../libs/ravastore.o \
		 rava.a

$(LUA_LIBS): $(LUA_DEPS)
	@$(EACH) MKDIR libs/luajit/
	@$(MKDIR) libs/luajit/
	@$(EACH) COPY $(LUA_DEPS)
	@$(EACH) TO libs/luajit/
	@$(COPY) $(LUA_DEPS) libs/luajit/
	@$(EACH) ED libs/luajit/bcsave.lua
	@sed -i'.bak' -e's/^Save.\+//' -e's/^  /\t/g' -e's/^File /\t/' -e's/\.$$//' \
		libs/luajit/bcsave.lua

$(LUA_DEPS):
	@$(EACH) MAKING $(DIR_DEPS_LUA)
	@$(MAKING) $(DIR_DEPS_LUA) CFLAGS="-fPIC"

$(LUV_LIBS): $(LUV_DEPS)
	@$(EACH) MKDIR libs/libuv/
	@$(MKDIR) libs/libuv/
	@$(EACH) COPY $(LUV_DEPS)
	@$(EACH) TO libs/libuv/
	@$(COPY) $(LUV_DEPS) libs/libuv/

$(LUV_DEPS):
	@cd $(DIR_DEPS_LUV) && ./autogen.sh
	@cd $(DIR_DEPS_LUV) && ./configure
	@$(EACH) MAKING $(DIR_DEPS_LUV)
	@$(MAKING) $(DIR_DEPS_LUV) CFLAGS="-fPIC"

deps: deps-git deps-luajit deps-libuv deps-src deps-rava

deps-git:
	@$(EACH) MAKING git-deps
	@git submodule update --init

deps-rava: deps-src $(RAVA_LIBS) $(RAVA_OBJS)
deps-src: deps-libuv deps-luajit $(RAVA_DEPS)
deps-luajit: $(LUA_LIBS)
deps-libuv: $(LUV_LIBS)

clean: clean-rava
	@$(EACH) REMOVE libs/ rava*
	@$(REMOVE) libs/ rava*

clean-all: clean clean-luajit clean-libuv clean-rava

clean-luajit:
	@$(EACH) CLEAN deps/luajit/
	@$(CLEAN) deps/luajit/
	@cd deps/luajit/ && git clean -dfx

clean-libuv:
	@$(EACH) CLEAN deps/libuv/
	@$(CLEAN) deps/libuv/
	@cd deps/libuv/ && git clean -dfx

clean-rava:
	@$(EACH) REMOVE libs/ravastore.* lua/rava.a lua/*.o lua/modules/*.o
	@$(REMOVE) libs/ravastore.* lua/rava.a lua/*.o lua/modules/*.o
	@$(EACH) CLEAN src/
	@$(CLEAN) src/
