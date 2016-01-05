local msg = {}
local function ansiesc(num)
	return (string.char(27) .. '[%dm'):format(num)
end

msg.out = io.stderr

msg.add = function(str)
	msg.out:write(str)
end

msg.line = function(str)
	msg.add("\n"..(str or ""))
end

msg.indent = function(str)
	msg.line("\t"..(str or ""))
end

msg.format = function(str, ...)
	msg.add(str:format(...))
end

msg.color = function(c, m, ...)
	msg.add("\n"..ansiesc(c).."["..m:upper().."]".. ansiesc(0) .. " "..table.concat({...} or {},"\t"))
end

msg.done = function(str)
	msg.add(" "..ansiesc(32)..(str or "Done.")..ansiesc(0))
end

msg.fail = function(str)
	msg.add(" "..ansiesc(31)..(str or "Failed!")..ansiesc(0))
end

msg.fatal = function(...)
	msg.color(31, "fatal", ..., "\n\n")
	os.exit(1)
end

msg.error = function(...)
	msg.color(31, "help", ...)
end

msg.warning = function(...)
	msg.color(33, "warn", ...)
end

msg.info = function(...)
	msg.color(32, "info", ...)
end

msg.contents = function(tbl)
	msg.line()

	for k, v in pairs(tbl) do
		msg.indent(tostring(k).."\t"..tostring(v))
	end

	msg.line()
end

msg.dump = function(...)
	local objs = {...}
	local dmp = function(dmp, obj, name, ind)
		local tp = type(obj)
		local ind = ind or ""
		local name = name or ""
		local val = "["..tp.."]"

		if tp == "string" then
			val = obj
		end

		msg.format("%s%s: %s\n", ind, name, val)

		if tp == "table" then
			if #name then
				name = name.."."
			end

			for k, v in pairs(obj) do
				dmp(dmp, v, k, ind.."\t")
			end
		end
	end

	for i = 1, #objs do
		msg.line()
		dmp(dmp, objs[i], "arg"..i)
	end
end


module(...)

return msg
