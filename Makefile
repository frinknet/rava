VERSION=2.0.5

export PREFIX=/usr/local

LUA_DEPS=deps/luajit/src/luajit deps/luajit/src/*.h deps/luajit/src/libluajit.a \
deps/luajit/src/jit/bcsave.lua
LUA_LIBS=libs/luajit/luajit libs/luajit/lua.h libs/luajit/lualib.h \
libs/luajit/luaconf.h libs/luajit/lauxlib.h libs/luajit/libluajit.a \
libs/luajit/bcsave.lua

UV_DEPS=deps/libuv/.libs/libuv.a deps/libuv/src/queue.h deps/libuv/include/*.h
UV_LIBS=libs/libuv/libuv.a libs/libuv/uv.h libs/libuv/queue.h

RAVA_OBJ=src/main.o src/rava.o src/ray.o
RAVA_LUA=lua/gen.lua lua/msg.lua lua/opt.lua lua/init.lua
RAVA_LIBS=lua/rava.a

DPREFIX=$(DESTDIR)$(PREFIX)
INSTALL_BIN=$(DPREFIX)/bin
INSTALL_DEP=rava

CCARGS=-Ilibs/luajit/ -Ilibs/libuv/ -lpthread

CC=gcc
LD=ld

all: deps-rava $(INSTALL_DEP)

$(INSTALL_DEP):
	@echo "==== Building Rava $(VERSION) ===="
	cd lua && ./rava.sh -csn --compile=../rava main.lua modules/*.lua
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

install: $(INSTALL_DEP)
	@sudo cp $(INSTALL_DEP) $(INSTALL_BIN)
	@echo "==== Installed Rava $(VERSION) to $(INSTALL_BIN) ===="

uninstall:
	@echo "==== Uninstalling Rava $(VERSION) from $(PREFIX) ===="
	rm -f $(INSTALL_BIN)/rava
	@echo "==== Uninstalled LuaJIT $(VERSION) from $(PREFIX) ===="

clean: clean-rava
	rm -rf libs/ rava*

clean-all: clean clean-luajit clean-libuv clean-rava

clean-luajit:
	$(MAKE) clean -C deps/luajit/
	cd deps/luajit/ && git clean -dfx

clean-libuv:
	$(MAKE) clean -C deps/libuv/
	cd deps/libuv/ && git clean -dfx

clean-rava:
	rm -f libs/ravastore.* lua/rava.a lua/*.o lua/modules/*.o
	$(MAKE) clean -C src

deps: deps-git deps-luajit deps-libuv deps-src deps-rava

deps-git:
	git submodule update --init

deps-rava: deps-src $(RAVA_LIBS) $(RAVA_LUA)
deps-src: deps-libuv deps-luajit $(RAVA_OBJ)
deps-luajit: $(LUA_LIBS)
deps-libuv: $(UV_LIBS)

$(RAVA_OBJ):
	@$(MAKE) -C src/

$(RAVA_LIBS) $(RAVA_LUA):
	@echo BYTECODE lua/init.lua.o
	@cd lua && ./rava.sh --bytecode=init init.lua init.lua.o
	@echo BUILD lua/rava.a
	@cd lua && ./rava.sh --build=rava ../src/main.o ../src/rava.o ../src/ray.o \
		init.lua.o gen.lua opt.lua msg.lua gen/bcsave.lua \
		../libs/libuv/libuv.a ../libs/luajit/libluajit.a
	@echo FILESTORE libs/ravastore.o
	cd lua && ./rava.sh --filestore=ravastore ../libs/ravastore.o rava.a

$(LUA_LIBS): $(LUA_DEPS)
	@echo COPY LuaJIT Library
	@mkdir -p libs/luajit/
	@cp $(LUA_DEPS) libs/luajit/
	@sed -i'.bak' -e's/^Save.\+//' -e's/^  /\t/g' -e's/^File /\t/' -e's/\.$$//' \
		libs/luajit/bcsave.lua

$(LUA_DEPS):
	@echo MAKE LuaJIT
	@$(MAKE) -C deps/luajit/

$(UV_LIBS): $(UV_DEPS)
	@echo COPY LibUV Library
	@mkdir -p libs/libuv/
	@cp $(UV_DEPS) libs/libuv/

$(UV_DEPS):
	@echo AUTOTOOLS LibUV
	@cd deps/libuv/ && ./autogen.sh
	@echo CONFIGURE LibUV
	@cd deps/libuv/ && ./configure
	@echo MAKE LibUV
	@$(MAKE) -C deps/libuv/
