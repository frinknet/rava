-- Macros. Run at compile time.
opt.add("m", "Process <".."%= macros %".."> in source before compiling", function()
	rava.addPreProcessor(function(code, file)
		local printed = false
		local newcode = code:gsub("%<%[%[(.-)%]%]%>", function(code)
			if code and file then
				if not printed then
					msg.info("Running Macros in ".. infile .."... ")

					printed = true
				end
				local fn, err = loadstring(code)
				if err then
					msg.fatal("Macro failed: "..tostring(err))
				end
				local ok, r = pcall(fn)
				if ok then
					return tostring(r)
				else
					msg.fatal("Macro failed to execute: "..tostring(r))
				end
			end
		end)

		if printed then
			print("Done.")
		end

		return newcode
	end)
end)
