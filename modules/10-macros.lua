-- Macros. Run at compile time.
opt.add("m", "Process <[[=macros]]> in source before compiling", function()
	rava.addPreHook(function(file)
		local code = rava.code[file]
		local printed = false

		rava.code[file] = code:gsub("%<%[%[(.-)%]%]%>", function(chunk)
			if chunk and file then
				if not printed then
					msg.info("Macros in " .. file)

					printed = true
				end

				local fn, err = loadstring(chunk:gsub("^=", "return "))

				if err then
					msg.fatal("Macro failed: "..tostring(err))
				end

				local ok, r = pcall(fn)

				if ok then
					return tostring(r)
				else
					msg.fatal("Macro failed: "..tostring(r))
				end
			end
		end)

		if printed then
			msg.done()
		end
	end)
end)
