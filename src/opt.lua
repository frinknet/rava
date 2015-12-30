local msg = require("src.msg")

local opt_act = {}
local opt_def = {}
local opt_seq = {}
local opt_lst = {}
local opt = {}

-- Parse options to populate
opt.parse = function(arg)
	-- track processed options to remove
	local rem = {}

	-- reset list
	opt_lst = {}

	-- loop through args
	for i, v in pairs(arg) do
		-- check for long form opts with assignments
		if string.sub(v, 1, 2) == "--" then
			table.insert(rem, i)

			local x = string.find(v, "=", 1, true)

			if x then
				opt_lst[string.sub(v, 3, x - 1)] = string.sub(v, x + 1)
			else
				opt_lst[string.sub(v, 3)] = true
			end

		-- check for short form opts as flags
		elseif #v > 1 and string.sub(v, 1, 1) == "-" then
			table.insert(rem, i)

			local y = 2

			-- work through option flags
			while (y <= #v) do
				opt_lst[string.sub(v, y, y)] = true
				y = y + 1
			end
		end
	end

	-- remove the processed options
	while rem[#rem] do
		table.remove(arg,rem[#rem])
		table.remove(rem,#rem)
	end
end

-- Add option definer
opt.add = function(opt, def, act)
	if def then
		opt_def[opt] = def
	end

	if act then
			local x = string.find(opt, "=", 1, true)

			if x then
				opt = string.sub(opt, 1, x - 1)
			end

			opt_act[opt] = act
	end

	table.insert(opt_seq, opt)
end

opt.show = function()
	for _, k in pairs(opt_seq) do
		local v = opt_def[k]

		if #k > 1 then
			k = "-"..k
		end

		if v then
			print("\t-"..k.."\t"..v)
		end
	end
end

opt.run = function(k, ...)
	if k == nil or k == "*all" then
		for _, k in pairs(opt_seq) do
			if opt_lst[k] then
				opt.run(k, ...)
			end
		end
	else
		if opt_act[k] then
			return opt_act[k](opt_lst[k], ...)
		end
	end
end

opt.flag = function(k)
	return opt_lst[k]
end

opt.parse(arg)

module(...)

return opt
