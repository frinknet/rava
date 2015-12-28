-- Load libs
msg = require("src.msg")
opt = require("src.opt")
rava = require("src.rava")

-- set version
APPNAME = "Rava Micro-Service Compiler"
VERSION = "2.0.1"
OPLEVEL = 1
BINNAME = "rava"
RAVABIN = arg[-1].." "..arg[0]

-- Help
opt.add("h", "Show this help dialog", function(r)
	print(APPNAME.." v"..VERSION.." - "..jit.version.."\n")
	print("\tUsage:\n")
	print("\t"..arg[0].." [opt] files objects\n")
	print("\tOptions:\n")
	opt.show()
	print("")
	print("\tExamples:\n")
	print("\t"..arg[0].." --exec test.lua")
	print("\t"..arg[0].." --eval \"print('hello world.')\"")
	print("\t"..arg[0].." --compile=appsvr app.lua server.a")
	print("\t"..arg[0].." --generate=main main.lua main.lua.o\n")
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
	rava.evalCode(...)
	os.exit(0)
end)

-- Run files instead of compiling
opt.add("exec", "Executes files in rava runtime environment", function(...)
	rava.execFiles(...)

	os.exit(0)
end)

-- Compile binary
opt.add("compile=binary", "Compile a binary from lua files", function(...)
	rava.compile(...)
	os.exit(0)
end)

-- Generate bytecode object
opt.add("generate=module", "Generate a lua file to bytecode object", function(name, ...)
	rava.generate(name, ...)
	os.exit(0)
end)

-- Run options
opt.run("*all")

-- Make sure we have enough to do something
if #arg > 0 then
	opt.run("exec", arg)
else
	opt.run("h", 0)
end
