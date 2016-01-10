VERSION=2.0.3

export PREFIX=/usr/local

LUA_DEPS=deps/luajit/src/luajit deps/luajit/src/*.h deps/luajit/src/libluajit.a \
deps/luajit/src/jit/bcsave.lua
LUA_LIBS=libs/luajit/luajit libs/luajit/lua.h libs/luajit/lualib.h \
libs/luajit/luaconf.h libs/luajit/lauxlib.h libs/luajit/libluajit.a \
libs/luajit/bcsave.lua

UV_DEPS=deps/libuv/.libs/* deps/libuv/include/*.h
UV_LIBS=libs/libuv/libuv.a libs/libuv/uv.h

RAVA_SRC=src/rava.c
RAVA_LUA=lua/gen.lua lua/msg.lua lua/opt.lua lua/init.lua
RAVA_LIBS=libs/rava.a

DPREFIX=$(DESTDIR)$(PREFIX)
INSTALL_BIN=$(DPREFIX)/bin
INSTALL_DEP=rava

CCARGS=-Ilibs/luajit/ -Ilibs/libuv/ -lpthread

CC=gcc
LD=ld

all: rava

rava: deps-rava
	@echo "==== Building Rava $(VERSION) ===="
	cd lua && ./rava.sh -csn --compile=rava main.lua modules/*.lua
	@echo "==== Success builting Rava $(VERSION) ===="

debug: deps-rava
	@echo "==== Generating Rava Debug ===="
	@echo 'require("main")' > ravadebug.lua
	lua/rava.sh --compile ravadebug.lua
	@echo 'if #arg > 0 then rava.exec(unpack(arg)) ' \
		'else msg.line("Usage: "..arg[0]:gsub("^.*/", "").." file.lua\\n\\n") ' \
		'end' > ravaexec.lua
	./ravadebug -cs --compile ravaexec.lua
	@echo 'if #arg > 0 then rava.eval(unpack(arg)) ' \
		'else msg.line("Usage: "..arg[0]:gsub("^.*/", "").." file.lua\\n\\n") ' \
		'end' > ravaeval.lua
	./ravadebug -cs --compile ravaeval.lua
	@echo 'msg.add("Compiled at <[[=os.date()]]>\\n")' > ravatime.lua
	./ravadebug -csm --compile ravatime.lua
	@echo "==== Generated Rava Debug ===="
	$(MAKE) $(INSTALL_DEP)

install: rava
	@sudo cp $(INSTALL_DEP) $(INSTALL_BIN)
	@echo "==== Installed Rava $(VERSION) to $(INSTALL_BIN) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm -f $(INSTALL_BIN)/rava
	@echo "==== Uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

clean:
	rm -rf libs/* lua/*.o src/*.o lua/modules/*.o rava*

clean-all: clean clean-luajit clean-libuv

clean-luajit:
	$(MAKE) clean -C deps/luajit/
	cd deps/luajit/ && git clean -dfx

clean-libuv:
	$(MAKE) clean -C deps/libuv/
	cd deps/libuv/ && git clean -dfx

deps: deps-luajit deps-libuv deps-rava

deps-luajit: $(LUA_LIBS)
deps-libuv: $(UV_LIBS)
deps-rava: $(RAVA_LIBS) $(RAVA_LUA)

$(RAVA_LIBS) $(RAVA_LUA): $(LUA_LIBS) $(UV_LIBS)
	@echo "==== Generating Rava Core ===="
	$(CC) -c src/rava.c $(CCARGS) -o src/rava.o
	$(CC) -c src/xuv.c $(CCARGS) -o src/xuv.o
	cd lua && ./rava.sh --bytecode=init init.lua init.lua.o
	cd lua && ./rava.sh --build=rava ../src/rava.o ../src/xuv.o \
		init.lua.o gen.lua opt.lua msg.lua bytecode.lua \
		../libs/libuv/libuv.a ../libs/luajit/libluajit.a
	cd lua && ./rava.sh --filestore=ravastore ../libs/ravastore.o rava.a
	@echo "==== Generated Rava Core ===="

$(LUA_LIBS): $(LUA_DEPS)
	mkdir -p libs/luajit/
	cp $+ libs/luajit/
	sed -i'.bak' -e's/^Save.\+//' -e's/^  /\t/g' -e's/^File /\t/' -e's/\.$$//' \
		libs/luajit/bcsave.lua

$(UV_LIBS): $(UV_DEPS)
	mkdir -p libs/libuv/
	cp $+ libs/libuv/

$(LUA_DEPS):
	$(MAKE) -C deps/luajit/

$(UV_DEPS):
	cd deps/libuv/ && ./autogen.sh
	cd deps/libuv/ && ./configure
	$(MAKE) -C deps/libuv/
