-- Compact using UPX
opt.add("c", "Compact output using 'upx -9'", function()
	gen.addPostHook(function(binary)
		msg.info("Compacting Binary... ")

		local status = os.execute("upx -9 "..binary.." > /dev/null")

		if status/256 == 0 then
			msg.done()
		else
			msg.fail()
		end
	end)
end)
