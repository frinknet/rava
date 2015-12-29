local bcsave = require("libs/bcsave").start
local preHooks = {}
local postHooks = {}
local mainCode = [[
#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
int main(int argc, char *argv[]) {
	int i;
	lua_State* L = luaL_newstate();
	if (!L) { printf("Error: LuaJIT unavailable!!\n"); }
	luaL_openlibs(L);
	lua_newtable(L);
	for (i = 0; i < argc; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, argv[i]);
		lua_settable(L, -3);
	}
	lua_setglobal(L, "arg");
	int ret = luaL_dostring(L, "require \"main\"");
	if (ret != 0) {
		printf("%s\n", lua_tolstring(L, -1, 0));
		return ret;
	}
	return 0;
}
]]
local objs = {}
local rava = {}

-- check if file exists
local function fileExists(file)
	if file then
		local f = io.open(file,"r")
		if f ~= nil then
			io.close(f)

			return true
		else
			return false
		end
	end
end

-- run Hooks
local function callHooks(hooks, ...)
	for i=1, #hooks do
		hooks[i](...)
	end
end

local function addHook(hooks, fn)
	for i = 1, #hooks do
		if hooks[i] == fn then
			return false
		end
	end

	table.insert(hooks, fn)

	return true
end

-- call through pipe to generate
local function generateCodeObject(path, name)
	local dir, file, ext= string.match(path, "(.-)([^/]-([^%.]+))$")
	local objpath = path
	local leavesym = ""

	name = name or (dir:gsub("^%./",""):gsub("^/",""):gsub("/",".") .. file:gsub("%.lua$",""))

	-- Run preprocessors on file
	callHooks(preHooks, path)

	-- generate lua object
	if path:match("%.lua$") then
		if (not opt.flag("n")) then
			-- load code to check it for errors
			local fn, err = loadstring(rava.code[path])

			-- die on syntax errors
			if not fn then
				msg.fatal(err)
			end
		end

		-- check for leave symbols flag
		if opt.flag("g") then
			leavesym = "-g"
		end

		objpath = path..".o"

		msg.info("Generating "..objpath)

		-- create CC line
		local f = io.popen(string.format(
			"%s -q %s --generate=%s %s %s",
			RAVABIN,
			leavesym,
			name,
			"-",
			objpath),"w")

		-- write code to generator
		f:write(rava.code[path])
		f:close()

		msg.done()
	end

	-- add object
	table.insert(objs, objpath)

	--reclame memory (probably overkill in most cases)
	rava.code[path] = true

	return objpath
end

-- list files in a directory
rava.scandir = function(dir)
	local r = {}
	for file in io.popen("ls -a '"..dir.."'"):lines() do
		table.insert(r, file)
	end

	return r
end

-- Add PreHooks to filter input
rava.addPreHook = function(fn)
	return addHook(preHooks, fn)
end

-- Add PostHooks to futher process output
rava.addPostHook = function(fn)
	return addHook(postHooks, fn)
end

-- Add files to the rava compiler
rava.addFile = function(...)
	for i=1, #arg do
		local file = arg[i]

		if rava.code[file] then
			break
		elseif not fileExists(file) then
			msg.warning("File not found: "..file)
		elseif file:match("%.lua$") then
			msg.info("Load "..file)

			local f, err = io.open(file,"r")
			local v = #rava.code

			if err then msg.fatal(err) end

			rava.code[file] = f:read("*all")

			f:close()

			msg.done()

			-- check if we need to setup main
			if v > 0 then
				generateCodeObject(file)
			else
				generateCodeObject(file, "main")
			end
		else
			generateCodeObject(file)
		end
	end
end

-- Add a string to the rava compiler
rava.addString = function(name, code)
	rava.code[name] = code

	generateCodeObject(name, name)
end

-- Evaluate code to run in realtime
rava.eval = function(...)
	local chunk = ""

	for x = 1, #arg do
		chunk = chunk.."\n"..arg[x]
	end

	local fn=loadstring(chunk)

	fn()
end

-- Execute external files
rava.exec = function(...)
	for x = 1, #arg do
		dofile(arg[x])
	end
end

-- Compile the rava object state to binary
rava.compile = function(binary, ...)
	-- make sure we have a name
	if binary == true then
		binary = "rava"
	end

	--load Lua Code
	rava.addFile(...)

	msg.info("Compiling Binary... ")

	local cinit = string.format([[
	%s -O%s -Wall -Wl,-E \
		-x c %s -x none %s \
		%s \
		-o %s -lm -ldl -flto ]],
		os.getenv("CC") or "gcc",
		OPLEVEL,
		"-",
		table.remove(objs, 1),
		os.getenv("CCARGS").." "..table.concat(objs, " "),
		binary)
	local b = io.popen(cinit, "w")

	if b:write(mainCode) then
		msg.done()
	else
		msg.fail()
	end

	b:close()

	-- run PostHooks
	callHooks(postHooks, binary)

	print("\n")
end

-- Generate an object file from lua files
rava.generate = function(name, ...)
	local calls = {}

	if name ~= true then
		table.insert(calls, "-n")
		table.insert(calls, name)
	end

	for i = 1, #arg do
		table.insert(calls, arg[i])
	end

	bcsave(unpack(calls))
end

-- code repository
rava.code = {}

return rava
