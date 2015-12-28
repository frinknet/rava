local bcsave = require("libs/bcsave").start
local preProcessors = {}
local postProcessors = {}
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
local mainObject = {}
local moreObjects = {}
local rava = {}

rava.code = {}

-- Run an external process and return output
rava.execute = function(cmd)
	local f = io.popen(cmd)
	local r = f:read("*all"):match("^%s*(.-)%s*$")

	f:close()

	return r
end

rava.scandir = function(dir)
	local r = {}
	for file in io.popen("ls -a '"..dir.."'"):lines() do
		table.insert(r, file)
	end

	return r
end

-- check if file exists
rava.fileExists = function(file)
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

-- Add PreProcessor
rava.addPreProcessor = function(fn)
	for i = 1, #preProcessors do
		if preProcessors[i] == fn then
			return true
		end
	end

	table.insert(preProcessors, fn)
end

-- Run PreProcessors
rava.runPreProcessors = function(file)
	if file ~= nil or file ~= "*all" then
		for _, proc in pairs(preProcessors) do
			rava.code[file] = proc(rava.code[file], file)
		end
	elseif #preProcessors ~= 0 then
		for i = 1, #arg do
			rava.runPreProcessors(arg[i])
		end
	end
end

-- Add PostProcessor
rava.addPostProcessor = function(fn)
	for i = 1, #postProcessors do
		if postProcessors[i] == fn then
			return true
		end
	end

	table.insert(postProcessors, fn)
end

-- Run PostProcessors
rava.runPostProcessors = function(binary)
	if #postProcessors ~= 0 then
		for _, proc in pairs(postProcessors) do
			proc(binary)
		end
	end
end

-- Build rava.code table from files
rava.loadCodeFiles = function(...)
	for i=1, #arg do
		if arg[i]:match("%.lua$") then
			local f, err = io.open(arg[i],"r")
	
			if err then
				msg.fatal(err)
			end
	
			rava.code[arg[i]] = f:read("*all")
	
			f:close()
		end
	end
end

-- Build rava.code table from string
rava.loadCodeString = function(name, code)
	rava.code[name] = code
end

-- actual generate call
rava.generate = function(name, ...)
	local calls = {}

	if name ~= true then
		table.insert(calls, "-n")
		table.insert(calls, name)
	end

	for i = 1, #arg do
		table.insert(calls, arg[i])
	end

	print(APPNAME.." v"..VERSION.." - "..jit.version.."\n")
	print("\tYou are running: "..arg[0].." --generate")
	print("\tThis is identical to: luajit -b\n")

	bcsave(unpack(calls))
end

-- call through pipe to generate
rava.generatePipe = function(infile, outfile, name)
	local dir, file, ext= string.match(infile, "(.-)([^/]-([^%.]+))$")
	local f

	name = name or (dir:gsub("^%./",""):gsub("^/",""):gsub("/",".") .. file:gsub("%.lua$",""))
	outfile = outfile..".o"

	if (not opt.flag("n")) and infile then
		local fn, err = loadstring(rava.code[infile])

		if fn then
			f = io.popen(string.format("%s -q --generate=%s %s %s", RAVABIN, name, "-", outfile),"w")
		else
			msg.fatal(err)
		end
	else
		f = io.popen(string.format("%s -q --generate=%s %s %s", RAVABIN, name, "-", outfile),"w")
	end

	f:write(rava.code[infile])
	f:close()

	return outfile
end

-- loop through lua files
rava.generateLuaObjects = function(...)
	msg.info("Generating Objects... ")

	mainObject = rava.generatePipe(arg[1], arg[1], "main")

	if #arg >= 2 then
		for i = 2, #arg do
			if arg[i]:match("%.lua$") then
				table.insert(moreObjects, generateLuaObject(arg[i], arg[i]))
			else
				table.insert(moreObjects, arg[i])
			end
		end
	end

	msg.done()
end

-- create binary from objects
rava.compileToBinary = function(initCode, outputBinary, mainObject, moreObjects)
	msg.info("Compiling Binary... ")

	local cinit = string.format([[
	%s -O%s -Wall -Wl,-E \
		-x c %s -x none %s \
		%s \
		-o %s -lm -ldl -flto ]],
		(os.getenv("CC") or "gcc"),
		OPLEVEL,
		"-",
		mainObject,
		os.getenv("CCARGS").." "..table.concat(moreObjects, " "),
		outputBinary)
	local b = io.popen(cinit, "w")

	if b:write(initCode) then
		msg.done()
	else
		msg.fail()
	end

	b:close()
end

rava.evalCode = function(...)
	local chunk = ""

	for x = 1, #arg do
		chunk = chunk.."\n"..arg[x]
	end

	local fn=loadstring(chunk)

	fn()
end

rava.execFiles = function(...)
	for x = 1, #arg do
		dofile(arg[x])
	end
end

rava.compile = function(name, ...)
	print(APPNAME.." v"..VERSION.." - "..jit.version)

	-- make sure we have a name
	if name == true then
		name = "rava"
	end

	--load Lua Code
	rava.loadCodeFiles(...)

	-- run PreProcessors
	rava.runPreProcessors()

	--compile Lua to Bytecode
	rava.generateLuaObjects(...)

	--compile C and object code
	rava.compileToBinary(mainCode, name, mainObject, moreObjects)

	-- run PostProcessors
	rava.runPostProcessors(name)

	print("\n")
end

return rava
