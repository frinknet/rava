VERSION=2.0.1

LUA_DEPS=deps/luajit/src/luajit deps/luajit/src/lua.h deps/luajit/src/lualib.h \
deps/luajit/src/luaconf.h deps/luajit/src/lauxlib.h deps/luajit/src/libluajit.a \
deps/luajit/src/jit/bcsave.lua
LUA_LIBS=libs/luajit libs/lua.h libs/lualib.h libs/luaconf.h libs/lauxlib.h \
libs/libluajit.a libs/bcsave.lua

LIBUV_DEPS=deps/libuv/libuv.la
LIBUV_LIBS=libs/libuv.la

RAVA_SRC=src/rava.c src/rava.lua src/msg.lua src/opt.lua src/init.lua
RAVA_DEPS=libs/rava.a

DPREFIX=$(DESTDIR)$(PREFIX)
INSTALL_BIN=$(DPREFIX)/bin
INSTALL_DEP=rava

CCARGS=-Ilibs/

CC=gcc
LD=ld
RAVA=libs/luajit src/main.lua

all: rava

rava: $(LUA_LIBS) $(RAVA_DEPS)
	@echo "==== Building Rava $(VERSION) ===="
	$(RAVA) -csnr --compile=rava src/main.lua modules/*.lua
	@echo "==== Successfully built Rava $(VERSION) ===="

debug: $(LUA_LIBS) $(RAVA_DEPS)
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

$(RAVA_DEPS): $(LUA_LIBS)
	@echo "==== Generating Rava Core ===="
	$(RAVA) --generate=init src/init.lua src/init.lua.o
	$(RAVA) --generate src/rava.lua src/rava.lua.o
	$(RAVA) --generate src/opt.lua src/opt.lua.o
	$(RAVA) --generate src/msg.lua src/msg.lua.o
	$(CC) -c src/rava.c -Ilibs/ -o src/rava.o
	$(LD) -r src/rava.o src/init.lua.o src/rava.lua.o \
		src/opt.lua.o src/msg.lua.o libs/libluajit.a -o libs/rava.a
	$(RAVA) --datastore=ravastore libs/ravastore.o libs/rava.a
	@echo "==== Generated Rava Core ===="

install: all
	@echo "==== Installing Rava $(VERSION) to $(PREFIX) ===="
	cp $+ $(INSTALL_BIN)
	@echo "==== Installed Rava $(VERSION) to $(PREFIX) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm -f $(INSTALL_BIN)/rava
	@echo "==== Uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

clean:
	rm -rf libs/* src/*.o modules/*.o rava*

clean-all: clean
	$(MAKE) clean -C deps/luajit/

$(LUA_LIBS): $(LUA_DEPS)
	cp $+ libs/

$(LIBUV_LIBS): $(LIBUV_DEPS)
	cp $+ libs/

$(LUA_DEPS):
	$(MAKE) -C deps/luajit/

$(LIBUV_DEPS):
	cd deps/libuv && sh autogen.sh
	cd deps/libuv && sh autogen.sh
	cd deps/libuv && sh comfigure
	$(MAKE) check -C deps/libuv/
