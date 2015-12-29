local bcsave = require("libs/bcsave").start
local preHooks = {}
local postHooks = {}
local maincode
local mainobj
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

	-- First file we add is always main
	if mainobj == nil then
		name = "main"
		mainobj = true
	end

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

		msg.info("Chunk '"..name.."' in "..objpath)

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
	if name == "main" then
		mainobj = objpath
	else
		table.insert(objs, objpath)
	end

	--reclame memory (probably overkill in most cases)
	rava.code[path] = true

	return objpath
end

-- list files in a directory
function rava.scandir(dir)
	local r = {}
	for file in io.popen("ls -a '"..dir.."'"):lines() do
		table.insert(r, file)
	end

	return r
end

-- Add PreHooks to filter input
function rava.addPreHook(fn)
	return addHook(preHooks, fn)
end

-- Add PostHooks to futher process output
function rava.addPostHook(fn)
	return addHook(postHooks, fn)
end

-- Add files to the rava compiler
function rava.addFile(...)
	local arg = {...}

	for i=1, #arg do
		local file = arg[i]

		if rava.code[file] then
			break
		elseif not fileExists(file) then
			msg.warning("File not found: "..file)

			break
		elseif file:match("%.lua$") then
			msg.info("Loading "..file)

			local f, err = io.open(file,"r")

			if err then msg.fatal(err) end

			rava.code[file] = f:read("*all")

			f:close()

			msg.done()
		end

		generateCodeObject(file)
	end
end

-- Add a string to the rava compiler
function rava.addString(name, code)
	rava.code[name] = code

	generateCodeObject(name, name)
end

-- Evaluate code to run in realtime
function rava.eval(...)
	local arg = {...}
	local chunk = ""

	for x = 1, #arg do
		chunk = chunk.."\n"..arg[x]
	end

	local fn=loadstring(chunk)

	fn()
end

-- Execute external files
function rava.exec(...)
	local arg = {...}

	for x = 1, #arg do
		dofile(arg[x])
	end
end

-- Compile the rava object state to binary
function rava.compile(binary, ...)
	-- make sure we have a name
	if binary == true then
		binary = "rava"
	end

	--load Lua Code
	rava.addFile(...)

	msg.info("Compiling Binary... ")

	local f, err = io.open("src/main.c", "r")

	if err then msg.fatal(err) end

	maincode = f:read("*all")

	f:close()

	local cinit = string.format([[
	%s -O%s -Wall -Wl,-E \
		-x c %s -x none %s \
		%s \
		-o %s -lm -ldl -flto ]],
		os.getenv("CC") or "gcc",
		OPLEVEL,
		"-",
		mainobj,
		os.getenv("CCARGS").." "..table.concat(objs, " "),
		binary)
	msg.warning(cinit)
	local b = io.popen(cinit, "w")

	if b:write(maincode) then
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
function rava.generate(name, ...)
	local arg = {...}
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

-- Generate binary data store
function rava.store(variable, store, ...)
	-- open store
	local out = io.open(store, "w+")
	local files = {...}

	out:write("local "..variable.." = {}\n")

	variable = variable or "files"

	for path in files do
		local dir, file, ext= string.match(path, "(.-)([^/]-([^%.]+))$")
		local data = f:read("*all")
		local ins = io.open(path, "r")

		if not ins then
			msg.fatal("Error reading " .. path)
		end

		ins:close()
		out:write(variable..'["'..dir:gsub("^%./",""):gsub("^/","")..file..'"] = "')

		for i = 1, #data do
			out:write(string.format("\\%i", string.byte(data, i)))
		end

		out:write('"\n')
	end

	out:write('"\nmodule(...)\n')
	out:write('return '..variable)
	out:close()

	bcsave(store, store..".o")
end

-- code repository
rava.code = {}

module(...)

return rava
