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

CC=gcc
LD=ld

all: rava rava.so

rava.so: deps
	@echo CP $@
	@cp src/rava.so .

rava: deps
	@echo RV --compile=rava
	@cd lua && ./rava.sh -q -csn --compile=../rava main.lua modules/*.lua
	@rm rava.a

install: $(INSTALL_DEP)
	@echo IN $(INSTALL_BIN)/rava
	@sudo cp $(INSTALL_DEP) $(INSTALL_BIN)

uninstall:
	@echo RM $(INSTALL_BIN)/rava
	@rm -f $(INSTALL_BIN)/rava

clean: clean-rava
	@echo CL rava
	@rm -rf libs/ rava*

clean-all: clean clean-luajit clean-libuv clean-rava

clean-luajit:
	@echo CL deps/luajit/
	@$(MAKE) clean -C deps/luajit/
	@cd deps/luajit/ && git clean -dfx

clean-libuv:
	@echo CL deps/libuv/
	@$(MAKE) clean -C deps/libuv/
	@cd deps/libuv/ && git clean -dfx

clean-rava:
	@echo RM deps/librava/
	@rm -f libs/ravastore.* lua/rava.a lua/*.o lua/modules/*.o
	@$(MAKE) clean -C src

deps: deps-git deps-luajit deps-libuv deps-src deps-rava

deps-git:
	@echo MK git dependencies
	@git submodule update --init

deps-rava: deps-src $(RAVA_LIBS) $(RAVA_OBJS)
deps-src: deps-libuv deps-luajit $(RAVA_DEPS)
deps-luajit: $(LUA_LIBS)
deps-libuv: $(LUV_LIBS)

$(RAVA_DEPS): $(LUA_LIBS) $(LUV_LIBS)
	@echo MK $@
	@$(MAKE) -C src/

$(RAVA_LIBS): $(RAVA_DEPS)
	@echo MK $@
	@cp $+ libs/

$(RAVA_OBJS): $(RAVA_LIBS)
	@echo MK lua/rava.a
	@cd lua && ./rava.sh -q --bytecode=init init.lua init.lua.o
	@cd lua && ./rava.sh -q --build=rava \
		init.lua.o \
		gen.lua \
		opt.lua \
		msg.lua \
		gen/bcsave.lua \
		../libs/ravamain.a
	@echo MK libs/ravastore.o
	@cd lua && ./rava.sh -q --filestore=ravastore ../libs/ravastore.o rava.a

$(LUA_LIBS): $(LUA_DEPS)
	@echo MK libs/luajit/
	@mkdir -p libs/luajit/
	@echo CP $(LUA_DEPS) libs/luajit/
	@cp $(LUA_DEPS) libs/luajit/
	@sed -i'.bak' -e's/^Save.\+//' -e's/^  /\t/g' -e's/^File /\t/' -e's/\.$$//' \
		libs/luajit/bcsave.lua

$(LUA_DEPS):
	@echo MK deps/luajit/
	@$(MAKE) CFLAGS="-fPIC" -C deps/luajit/

$(LUV_LIBS): $(LUV_DEPS)
	@echo MK libs/libuv/
	@mkdir -p libs/libuv/
	@echo CP $(LUV_DEPS) libs/libuv/
	@cp $(LUV_DEPS) libs/libuv/

$(LUV_DEPS):
	@echo AT deps/libuv/
	@cd deps/libuv/ && ./autogen.sh
	@echo CF deps/libuv/
	@cd deps/libuv/ && ./configure
	@echo MK deps/libuv/
	@$(MAKE) CFLAGS="-fPIC" -C deps/libuv/
