-- Load libs
msg = require("src.msg")
opt = require("src.opt")
rava = require("src.rava")

-- set version
APPNAME = "Rava Micro-Service Compiler"
VERSION = "2.0.1"
OPLEVEL = 1
RAVABIN = arg[-1].." "..arg[0]

-- Help
opt.add("h", "Show this help dialog", function(r)
	print(APPNAME.." v"..VERSION.." - "..jit.version.."\n")
	print("\tUsage:\n")
	print("\t"..RAVABIN.." [opt] files objects\n")
	print("\tOptions:\n")
	opt.show()
	print("")
	print("\tExamples:\n")
	print("\t"..RAVABIN.." --exec test.lua")
	print("\t"..RAVABIN.." --eval \"print('hello world.')\"")
	print("\t"..RAVABIN.." --compile=appsvr app.lua server.a")
	print("\t"..RAVABIN.." --generate=main main.lua main.lua.o\n")
	os.exit(r == false)
end)

-- No output, except Errors..
opt.add("q", "Quiet for the most part, excludes Errors", function()
	function msg.warning(...) end
	function msg.info(...) end
	function msg.done(...) end
	function msg.fail(...) end
	function print(...) end
end)

-- No output at all.
opt.add("Q", nil, function()
	function msg.fatal() end
	function msg.error(...) end
	function msg.warning(...) end
	function msg.info(...) end
	function msg.done(...) end
	function msg.fail(...) end
	function print(...) end
end)

-- No Check file
opt.add("n", "Don't check the source file for syntax errors")

-- Debug symbols
opt.add("g", "Leave debug sybols intact in lua object files")

local modules = rava.scandir("modules")

for _,mod in pairs(modules) do
	local mod, match = mod:gsub(".lua$", "")
	if match == 1 then
		require("modules/"..mod)
	end
end

-- Run lua from commandline
opt.add("eval", "Evaluates a string lua code", function(...)
	rava.eval(...)
	os.exit(0)
end)

-- Run files instead of compiling
opt.add("exec", "Executes files in rava runtime environment", function(...)
	rava.exec(...)

	os.exit(0)
end)

-- Generate binary data store
opt.add("store=variable", "Generate a lua data store of binary files", function(...)
	print(APPNAME.." v"..VERSION.." - "..jit.version.."\n")

	rava.store(...)
	os.exit(0)
end)

-- Compile binary
opt.add("compile=binary", "Compile a binary from lua files", function(...)
	print(APPNAME.." v"..VERSION.." - "..jit.version)
	print("\tYou are running: "..RAVABIN.." --generate")
	print("\tThis is identical to: luajit -b\n")

	rava.compile(...)
	os.exit(0)
end)

-- Generate bytecode object
opt.add("generate=module", "Generate a lua file to bytecode object", function(...)
	local arg = {...}

	if #arg < 2 then
		print(APPNAME.." v"..VERSION.." - "..jit.version.."\n")
		print("\tUsage: "..RAVABIN.." --generate [opt]\n")
	end

	rava.generate(...)
	os.exit(0)
end)

-- Run options
opt.run("*all", unpack(arg))

-- Make sure we have enough to do something
if #arg > 0 then
	opt.run("exec", unpack(arg))
else
	opt.run("h", 0)
end
