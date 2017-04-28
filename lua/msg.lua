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

msg.list = function(...)
	local ins = {...}
	local objs = {}

	local function lst(obj, name, ind)
		local tp = type(obj)
		local ind = ind or ""
		local name = name or ""
		local val = "["..tp.."]"

		if tp == "string" or tp == "number" then
			val = obj
		end

		msg.format("%s%s: %s\n", ind, name, val)

		if tp == "userdata" then
			obj = getmetatable(obj)
			tp = type(obj)
		end

		if tp == "table" then
			for i = 1, #objs do
				if obj == objs[i] then
					return
				end
			end

			table.insert(objs, obj)

			for k, v in pairs(obj) do
				lst(v, k, ind.."\t")
			end
		end
	end

	msg.line()

	if #ins > 1 then
		lst(ins)
	else
		lst(ins[1])
	end
end

module(...)

return msg
