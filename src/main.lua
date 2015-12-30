-- Load libs
msg = require("src.msg")
opt = require("src.opt")
rava = require("src.rava")

-- set version
APPNAME = "Rava Micro-Service Compiler"
VERSION = "2.0.1"
OPLEVEL = 1
RAVABIN = arg[0]

-- Make sure ravabin is defined properly
if arg[-1] ~= nil then
	RAVABIN = arg[-1].." "..arg[0]
end

-- Help
opt.add("h", "Show this help dialog", function(r)
	msg.format("%s v%s - %s", APPNAME, VERSION, jit.version)
	msg.line("\tUsage:\n")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." [opt] files objects\n")
	msg.line("\tOptions:\n")
	opt.show()
	msg.line("")
	msg.line("\tExamples:\n")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --exec test.lua")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --eval \"print('hello world.')\"")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --compile=appsvr app.lua server.a")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --generate=main main.lua main.lua.o\n")
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
	function msg.add() end
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
opt.add("eval", "Evaluates a string lua code", function(flag, ...)
	rava.eval(...)
	os.exit(0)
end)

-- Run files instead of compiling
opt.add("exec", "Executes files in rava runtime environment", function(flag, ...)
	rava.exec(...)

	os.exit(0)
end)

-- Generate binary data store
opt.add("store=variable", "Generate a lua data store of binary files", function(name, store, ...)
	msg.format("%s v%s - %s", APPNAME, VERSION, jit.version)

	rava.store(name, store, ...)
	os.exit(0)
end)

-- Compile binary
opt.add("compile=binary", "Compile a binary from lua files", function(binary, alt, ...)
	if alt == nil then
		return opt.run("h")
	end

	if binary == true then
		binary = alt:gsub("%..-$", "")
	end

	msg.format("%s v%s - %s", APPNAME, VERSION, jit.version)
	msg.line("Compiling "..binary.." from:")
	msg.dump(...)
	rava.compile(binary, nil, ...)
	msg.line()

	os.exit(0)
end)

-- Generate bytecode object
opt.add("generate=module", "Generate a lua file to bytecode object", function(module, ...)
	local arg = {...}

	if #arg < 2 then
		msg.format("%s v%s - %s", APPNAME, VERSION, jit.version)
		msg.line("\tUsage: "..RAVABIN:gsub("^.*/", "").." --generate [opt]\n")
	end

	rava.generate(module, ...)
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
