-- Load libs
msg = require("msg")
opt = require("opt")
gen = require("gen")

-- set version
APPNAME = "Rava Micro-Service Compiler"
VERSION = "2.4.13"
LICENSE = "MIT license"
COPYYEAR = "2015-2017"
COPYNAME = "FrinkNET and Lemurs"
OPLEVEL = 1
RAVABIN = arg[0]

-- Make sure ravabin is defined properly
if arg[-1] ~= nil then
	RAVABIN = arg[-1].." "..arg[0]
end

-- Help
opt.add("h", "Show this help dialog", function(r)
	msg.header()
	msg.line("\tUsage:\n")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." [opt] files objects\n")
	msg.line("\tOptions:\n")
	opt.show()
	msg.line()
	msg.line("\tExamples:\n")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --exec test.lua")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --eval \"print('hello world.')\"")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --compile=appsvr app.lua server.a")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --bytecode=main main.lua main.lua.o\n")
	msg.line()
	os.exit(r == false)
end)

-- No output, except Errors..
opt.add("q", "Quiet except for errors", function()
	function msg.warning(...) end
	function msg.info(...) end
	function msg.done(...) end
	function msg.fail(...) end
	function msg.line(...) end
	function print(...) end
end)

-- No output at all.
opt.add("Q", "Quelch everything including error", function()
	function msg.add() end
	function print(...) end
end)

-- No Check file
opt.add("n", "Don't check the source file for syntax errors")

-- Debug symbols
opt.add("g", "Leave debug sybols intact in lua object files")

local modules = gen.scandir("modules")

for _,mod in pairs(modules) do
	local mod, match = mod:gsub(".lua$", "")
	if match == 1 then
		require("modules/"..mod)
	end
end

-- Run lua from commandline
opt.add("eval", "Evaluates a string lua code", function(flag, ...)
	gen.eval(...)
	os.exit(0)
end)

-- Run files instead of compiling
opt.add("exec", "Executes files in rava runtime environment", function(flag, ...)
	gen.exec(...)

	os.exit(0)
end)

-- Build library
opt.add("build=name", "Build a lua library from files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	name = name:gsub("%.[ao]$", ""):gsub("%.lua$", "")

	msg.header()
	msg.format("Building %s.a library from:\n\n", name)
	msg.list(file, ...)
	gen.build(name, file, ...)
	msg.line("\n\n")

	os.exit(0)
end)

-- Compile binary
opt.add("compile=name", "Compile a binary from lua files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	msg.header()
	msg.format("Compiling %s from:\n\n", name)
	msg.list(...)
	gen.compile(name, file, ...)
	msg.line("\n\n")

	os.exit(0)
end)

-- Generate bytecode object
opt.add("bytecode=name", "Generate a lua file to bytecode object", function(name, file, ...)
	if file == nil then
		msg.format("%s v%s - %s\n", APPNAME, VERSION, jit.version)
		msg.indent("Usage: "..RAVABIN:gsub("^.*/", "").." --bytecode [opt]\n")
		gen.bytecode()
		msg.line()
	end

	if name == true then
		name = file:gsub("%..-$", ""):gsub("/",".")
	end

	gen.bytecode("-n", name, file, ...)

	os.exit(0)
end)

-- Generate binary data store
opt.add("filestore=name", "Generate a lua data store of binary files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	gen.filestore(name, file, ...)

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
