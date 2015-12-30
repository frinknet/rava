VERSION=2.0.1

export PREFIX=/usr/local

LUA_DEPS=deps/luajit/src/luajit deps/luajit/src/*.h deps/luajit/src/libluajit.a \
deps/luajit/src/jit/bcsave.lua
LUA_LIBS=libs/luajit/luajit libs/luajit/lua.h libs/luajit/lualib.h \
libs/luajit/luaconf.h libs/luajit/lauxlib.h libs/luajit/libluajit.a \
libs/luajit/bcsave.lua

LIBUV_DEPS=deps/libuv/.libs/libuv.a deps/libuv/include/*
LIBUV_LIBS=libs/libuv/libuv.a

RAVA_SRC=src/rava.c src/rava.lua src/msg.lua src/opt.lua src/init.lua
RAVA_LIBS=libs/rava.a

DPREFIX=$(DESTDIR)$(PREFIX)
INSTALL_BIN=$(DPREFIX)/bin
INSTALL_DEP=rava

CCARGS=-Ilibs/luajit/

CC=gcc
LD=ld
RAVA=libs/luajit/luajit src/main.lua

all: rava

rava: deps-rava
	@echo "==== Building Rava $(VERSION) ===="
	$(RAVA) -csn --compile=rava src/main.lua modules/*.lua
	@echo "==== Success builting Rava $(VERSION) ===="

debug: deps-rava
	@echo "==== Generating Rava Debug ===="
	@echo 'require("src.main")' > ravadebug.lua
	$(RAVA) --compile ravadebug.lua
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

install:
	@echo "==== Installing Rava $(VERSION) to $(PREFIX) ===="
	cp $(INSTALL_DEP) $(INSTALL_BIN)
	@echo "==== Installed Rava $(VERSION) to $(PREFIX) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm -f $(INSTALL_BIN)/rava
	@echo "==== Uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

clean:
	rm -rf libs/* src/*.o modules/*.o rava*

clean-all: clean clean-luajit clean-libuv

clean-luajit:
	$(MAKE) clean -C deps/luajit/
	cd deps/luajit/ && git clean -dfx

clean-libuv:
	$(MAKE) clean -C deps/libuv/
	cd deps/libuv/ && git clean -dfx

deps: deps-luajit deps-libuv deps-rava

deps-luajit: $(LUA_LIBS)
deps-libuv: $(LIBUV_LIBS)
deps-rava: $(RAVA_LIBS)

$(RAVA_LIBS): $(LUA_LIBS) $(LIBUV_LIBS)
	@echo "==== Generating Rava Core ===="
	$(CC) -c src/rava.c $(CCARGS) -o src/rava.o
	$(RAVA) --bytecode=init src/init.lua src/init.lua.o
	$(RAVA) --build=rava src/rava.o src/init.lua.o src/rava.lua \
		src/opt.lua src/msg.lua libs/luajit/bcsave.lua \
		libs/luajit/libluajit.a libs/libuv/libuv.a
	$(RAVA) --datastore=ravastore libs/ravastore.o rava.a
	@echo "==== Generated Rava Core ===="

$(LUA_LIBS): $(LUA_DEPS)
	mkdir -p libs/luajit/
	cp $+ libs/luajit/
	sed -i'.bak' -e's/^Save.\+//' -e's/^  /\t/g' -e's/^File /\t/' -e's/\.$$//' libs/luajit/bcsave.lua

$(LIBUV_LIBS): $(LIBUV_DEPS)
	mkdir -p libs/libuv/
	cp $+ libs/libuv/

$(LUA_DEPS):
	$(MAKE) -C deps/luajit/

$(LIBUV_DEPS):
	cd deps/libuv && sh autogen.sh
	cd deps/libuv && sh autogen.sh
	cd deps/libuv && sh configure
	$(MAKE) check -C deps/libuv/
