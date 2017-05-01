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
LD:=ld
CP:=cp
RM:=rm -rf
MD:=mkdir -p
MK:=$(MAKE) -C
CL:=$(MAKE) clean -C

EACH=utils/each.sh
RAVA=cd lua && ../utils/rava.sh -q

all: rava rava.so

rava.so: deps
	@$(EACH) CP src/rava.so
	@$(EACH) TO rava.so
	@$(CP) src/rava.so .

rava: deps
	@$(EACH) RAVA \
		main.lua \
		config.lua \
		gen/bcsave.lua \
		opts/*.lua
	@$(EACH) BINARY rava
	@$(RAVA) -sc --binary=../rava \
		main.lua \
		config.lua \
		gen/bcsave.lua \
		opts/*.lua

test: rava
	@$(EACH) RAVA hello
	@./rava --binary=hello example/hello.lua

install: $(INSTALL_DEP)
	@$(EACH) INSTALL $(INSTALL_BIN)/rava
	@sudo $(CP) $(INSTALL_DEP) $(INSTALL_BIN)

uninstall:
	@$(EACH) RM $(INSTALL_BIN)/rava
	@$(RM) $(INSTALL_BIN)/rava

$(RAVA_DEPS): $(LUA_LIBS) $(LUV_LIBS)
	@$(EACH) MK src/
	@$(MK) src/

$(RAVA_LIBS): $(RAVA_DEPS)
	@$(EACH) CP $@
	@$(CP) $+ libs/

$(RAVA_OBJS): $(RAVA_LIBS)
	@$(EACH) RAVA lua/init.lua
	@$(EACH) BYTECODE lua/init.lua.o
	@$(RAVA) --bytecode=init init.lua init.lua.o
	@$(EACH) RAVA \
		init.lua.o \
		opt.lua \
		msg.lua \
		../libs/ravamain.a
	@$(EACH) LIBRARY lua/rava.a
	@$(RAVA) --library=rava \
		init.lua.o \
		opt.lua \
		msg.lua \
		../libs/ravamain.a
	@$(EACH) RAVA \
		 ../libs/ravastore.o \
		 rava.a
	@$(EACH) FILESTORE libs/ravastore.o
	@$(RAVA) --filestore=ravastore \
		 ../libs/ravastore.o \
		 rava.a

$(LUA_LIBS): $(LUA_DEPS)
	@$(EACH) MD libs/luajit/
	@$(MD) libs/luajit/
	@$(EACH) CP $(LUA_DEPS)
	@$(EACH) TO libs/luajit/
	@$(CP) $(LUA_DEPS) libs/luajit/
	@$(EACH) ED libs/luajit/bcsave.lua
	@sed -i'.bak' -e's/^Save.\+//' -e's/^  /\t/g' -e's/^File /\t/' -e's/\.$$//' \
		libs/luajit/bcsave.lua

$(LUA_DEPS):
	@$(EACH) MK $(DIR_DEPS_LUA)
	@$(MK) $(DIR_DEPS_LUA) CFLAGS="-fPIC"

$(LUV_LIBS): $(LUV_DEPS)
	@$(EACH) MD libs/libuv/
	@$(MD) libs/libuv/
	@$(EACH) CP $(LUV_DEPS)
	@$(EACH) TO libs/libuv/
	@$(CP) $(LUV_DEPS) libs/libuv/

$(LUV_DEPS):
	@cd $(DIR_DEPS_LUV) && ./autogen.sh
	@cd $(DIR_DEPS_LUV) && ./configure
	@$(EACH) MK $(DIR_DEPS_LUV)
	@$(MK) $(DIR_DEPS_LUV) CFLAGS="-fPIC"

deps: deps-git deps-luajit deps-libuv deps-src deps-rava

deps-git:
	@$(EACH) MK git-deps
	@git submodule update --init

deps-rava: deps-src $(RAVA_LIBS) $(RAVA_OBJS)
deps-src: deps-libuv deps-luajit $(RAVA_DEPS)
deps-luajit: $(LUA_LIBS)
deps-libuv: $(LUV_LIBS)

clean: clean-rava
	@$(EACH) RM libs/ rava*
	@$(RM) libs/ rava*

clean-all: clean clean-luajit clean-libuv clean-rava

clean-luajit:
	@$(EACH) CL deps/luajit/
	@$(CL) deps/luajit/
	@cd deps/luajit/ && git clean -dfx

clean-libuv:
	@$(EACH) CL deps/libuv/
	@$(CL) deps/libuv/
	@cd deps/libuv/ && git clean -dfx

clean-rava:
	@$(EACH) RM libs/ravastore.* lua/rava.a lua/*.o lua/opts/*.o
	@$(RM) libs/ravastore.* lua/rava.a lua/*.o lua/opts/*.o
	@$(EACH) CL src/
	@$(CL) src/
