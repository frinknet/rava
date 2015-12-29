-- Macros. Run at compile time.
opt.add("r", "Recursively include files", function()
	rava.addPreHook(function(file)
		-- load code to parse
		local code = rava.code[file]
		local findreq = function(chunk, m)
			chunk:gsub(m, function(line, file)
				file = file:gsub("[.]", "/")..".lua"

				rava.addFile(file)
		
				return line
			end)
		end

		if code == nil then
			return
		elseif file:match("%.lua$") then
			-- Process all required patters
			findreq(code, '(require%s-%(%s-"(.-)"%s-%))')
			findreq(code, '(require%s-"(.-)")')
			findreq(code, "(require%s-%(%s-'(.-)'%s-%))")
			findreq(code, "(require%s-'(.-)')")
		end
	end)
end)

