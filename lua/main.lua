-- provide instanciated inbuilt libs
opt = require "opt"
msg = require "msg"

local bytecode = require "gen.bcsave"
local preHooks = {}
local postHooks = {}
local ccargs = os.getenv("CCARGS") or ""
local mainobj
local objs = {}
local gen = {}

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

-- run hooks array
local function callHooks(hooks, ...)
	for i=1, #hooks do
		hooks[i](...)
	end
end

-- add hook to hooks array
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
local function generateCodeObject(file)
	local objfile = file
	local leavesym = ""
	local name = file:gsub("%..-$", ""):gsub("/",".")

	-- First file we add is always main
	if mainobj == nil then
		name = "main"
		mainobj = true
	end

	-- Run preprocessors on file
	callHooks(preHooks, file)

	-- generate lua object
	if file:match("%.lua$") then
		if (not opt.flag("n")) then
			-- load code to check it for errors
			local fn, err = loadstring(gen.code[file])

			-- die on syntax errors
			if not fn then
				msg.fatal(err)
			end
		end

		-- check for leave symbols flag
		if opt.flag("g") then
			leavesym = "-g"
		end

		objfile = file..".o"

		msg.info("'"..name.."' = "..file)

		-- create CC line
		local f = io.popen(string.format(
			"%s -q %s --bytecode=%s %s %s",
			RAVABIN,
			leavesym,
			name,
			"-",
			objfile),"w")

		-- write code to generator
		f:write(gen.code[file])
		f:close()

		msg.done()
	else
		msg.info("Adding object "..file)
		msg.done()
	end

	-- add object
	if name == "main" then
		mainobj = objfile
	else
		table.insert(objs, objfile)
	end

	--reclame memory (probably overkill in most cases)
	gen.code[file] = true

	return objfile
end

-- list files in a directory
gen.scandir = function(dir)
	local r = {}
	for file in io.popen("ls -a '"..dir.."' 2>/dev/null"):lines() do
		table.insert(r, file)
	end

	return r
end

-- Add PreHooks to filter input
gen.addPreHook = function(fn)
	return addHook(preHooks, fn)
end

-- Add PostHooks to futher process output
gen.addPostHook = function(fn)
	return addHook(postHooks, fn)
end

-- Add files to the rava compiler
gen.addFile = function(...)
	local arg = {...}

	for i=1, #arg do
		-- normalize filename
		local file = arg[i]:gsub("^%./",""):gsub("^/","")

		if gen.code[file] then
			break
		elseif not fileExists(file) then
			msg.warning("Failed to add module: "..file)

			break
		elseif file:match("%.lua$") then
			msg.info("Loading "..file)

			local f, err = io.open(file,"r")

			if err then msg.fatal(err) end

			gen.code[file] = f:read("*all")

			f:close()

			msg.done()
		end

		generateCodeObject(file)
	end
end

-- Add a string to the rava compiler
gen.addString = function(name, code)
	name = name:gsub("^%./",""):gsub("^/","")

	gen.code[name] = code

	generateCodeObject(name)
end

-- Evaluate code to run in realtime
gen.eval = function(...)
	local arg = {...}
	local chunk = ""

	for x = 1, #arg do
		chunk = chunk.."\n"..arg[x]
	end

	local fn=loadstring(chunk)

	fn()
end

-- Execute external files
gen.exec = function(...)
	local arg = {...}

	for x = 1, #arg do
		dofile(arg[x])
	end
end

gen.build = function(name, ...)
	mainobj = true
	name = name..".a"

	--load Lua Code
	gen.addFile(...)

	msg.info("Building "..name.." library... ")

	-- Construct compiler call
	local ccall = string.format([[
		%s -r -static %s \
		-o %s ]],
		os.getenv("LD") or "ld",
		table.concat(objs, " "),
		name)

	-- Call compiler
	os.execute(ccall)

	msg.done()

	-- run PostHooks
	callHooks(postHooks, name)
end

-- Compile the rava object state to binary
gen.compile = function(name, ...)
	--load Lua Code
	gen.addFile(...)

	msg.info("Compiling Binary... ")

	local f, err = io.open(name..".a", "w+")
	local files = require("gen"..".ravastore")

	f:write(files["rava.a"])
	f:close()

	-- Construct compiler call
	local ccall = string.format([[
	%s -O%s -Wall -Wl,-E \
		-x none %s -x none %s \
		%s \
		-o %s -lm -ldl -flto -lpthread ]],
		os.getenv("CC") or "gcc",
		OPLEVEL,
		name..".a",
		mainobj,
		ccargs.." "..table.concat(objs, " "),
		name)

	-- Call compiler
	os.execute(ccall)

	msg.done()

	-- run PostHooks
	callHooks(postHooks, name)
end

-- Generate an object file from lua files
gen.bytecode = bytecode.start

-- Generate binary filestore
gen.filestore = function(name, store, ...)
	store = store:gsub("%.[^%.]+$", "")

	-- open store
	local out = io.open(store..".lua", "w+")
	local files = {...}

	-- start name
	out:write("local "..name.." = {}\n")

	-- loop through files
	for _, file in pairs(files) do
		local ins = io.open(file, "r")

		-- test for failed open
		if not ins then
			msg.fatal("Error reading " .. file)
		end

		-- add file entry to table
		out:write(name..'["'..file..'"] = "')

		-- write all data to store in memory safe way
		repeat
			local char = ins:read(1)

			if char ~= nil then
				out:write(string.format("\\%i", char:byte()))
			end
		until char == nil

		ins:close()
		out:write('"\n')
	end

	-- add module code
	out:write('\nmodule(...)\n')
	out:write('return '..name)
	out:close()

	gen.bytecode(store..".lua", store..".o")
end

gen.start = function(env)
	-- loop through and add globals
	for k, v in pairs(env) do
		_G[k] = v
	end

	_G.RAVABIN = os.getenv("RAVABIN")
	_G.VERSION = _G.VERSION.." // "..jit.version

	-- Make sure ravabin is defined properly
	if not _G.RAVABIN then
		if arg[-1] ~= nil then
			_G.RAVABIN = arg[-1].." "..arg[0]
		else
		_G.RAVABIN = arg[0]
		end
	end
	
	for _,mod in pairs(MODULES) do
		require("modules."..mod)
	end
	
	-- Run options
	opt.run("*all", unpack(arg))
	
	-- Make sure we have enough to do something
	if #arg > 0 then
		gen.exec(unpack(arg))
	else
		opt.run("h", 0)
	end
end

-- code repository
gen.code = {}

gen.start(require "config")
