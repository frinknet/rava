local msg = {}
local function ansiesc(num)
	return (string.char(27) .. '[%dm'):format(num)
end

local function message(c, m, ...)
	io.stderr:write("\n"..ansiesc(c).."["..m:upper().."]".. ansiesc(0) .. " "..table.concat({...},"\t"))
end

msg.done = function(msg)
	io.stderr:write(" "..ansiesc(32)..(msg or "Done.")..ansiesc(0))
end

msg.fail = function(msg)
	io.stderr:write(" "..ansiesc(31)..(msg or "Failed!")..ansiesc(0))
end

msg.line = function(msg)
	io.stderr:write(msg)
end

msg.printf = function(msg, ...)
	io.write(msg:format(...).."\n")
end

msg.fatal = function(...)
	message(31, "fatal", ..., "\n\n")
	os.exit(1)
end

msg.error = function(...)
	message(31, "error", ...)
end

msg.warning = function(...)
	message(33, "warning", ...)
end

msg.info = function(...)
	message(32, "info", ...)
end

function msg.dump(...)
	local arg = {...}

	for i=1, #arg do
		print(arg[i])
	end

	os.exit(1)
end

return msg
