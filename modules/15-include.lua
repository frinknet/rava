-- Macros. Run at compile time.
opt.add("r", "Recursively include files", function()
	rava.addPreHook(function(file)
		-- load code to parse
		local code = rava.code[file]
		local findreq = function(chunk, m)
			chunk:gsub(m, function(line, file)
				msg.info("Recursive: "..line)
				rava.addFile(file..".lua")
		
				return line
			end)
		end

		-- Process all required patters
		findreq(code, '(require%s+%("(.-)"%))')
		findreq(code, '(require%s+"(.-)")')
		findreq(code, "(require%s+%('(.-)'%))")
		findreq(code, "(require%s+'(.-)')")
	end)
end)

