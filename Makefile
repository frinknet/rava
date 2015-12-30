VERSION=2.0.1

export PREFIX= /usr/local

LUA_DEPS=deps/luajit/src/luajit deps/luajit/src/lua.h deps/luajit/src/lualib.h \
deps/luajit/src/luaconf.h deps/luajit/src/lauxlib.h deps/luajit/src/libluajit.a \
deps/luajit/src/jit/bcsave.lua
LUA_LIBS=libs/luajit libs/lua.h libs/lualib.h libs/luaconf.h libs/lauxlib.h \
libs/libluajit.a libs/bcsave.lua
RAVA_SRC=src/rava.c src/rava.lua src/msg.lua src/opt.lua src/init.lua
RAVA_DEPS=libs/rava.a libs/ravastore.o

DPREFIX= $(DESTDIR)$(PREFIX)
INSTALL_BIN=   $(DPREFIX)/bin
INSTALL_DEP=   rava

CCARGS=-Ilibs/

CC=gcc
LD=ld
RAVA=libs/luajit src/main.lua

default all $(INSTALL_DEP): $(RAVA) $(RAVA_DEPS)
	$(MAKE) git
	@echo "==== Building Rava $(VERSION) ===="
	$(RAVA) -csnr --compile=rava src/main.lua modules/*.lua
	@echo "==== Successfully built Rava $(VERSION) ===="

install: $(INSTALL_DEP)
	@echo "==== Installing Rava $(VERSION) to $(PREFIX) ===="
	cp $+ $(INSTALL_BIN)
	@echo "==== Installed Rava $(VERSION) to $(PREFIX) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm -f $(INSTALL_BIN)/rava
	@echo "==== Uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

git:
	@echo "==== Update Git Repositories ===="
	#git pull
	git submodule init
	git submodule update
	@echo "==== Update git $(VERSION) ===="

clean:
	rm -rf libs/* src/*.o modules/*.o rava*

clean-all: clean
	$(MAKE) clean -C deps/luajit/

debug: $(LUA_LIBS) $(RAVA) $(RAVA_DEPS)
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
	$(RAVA) -csnr --compile=rava src/main.lua modules/*.lua
	@echo "==== Generated Rava Debug ===="

$(RAVA_DEPS): $(RAVA) $(RAVA_SRC) $(LUA_LIBS)
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

$(LUA_LIBS): $(LUA_DEPS)
	cp $+ libs/

$(LUA_DEPS):
	$(MAKE) -C deps/luajit/
