-- Strip
opt.add("s", "Use -Os and strip output file using 'strip --strip-all'", function()
	gen.addPostHook(function(binary)
		optimisationlevel = "s"

		msg.info("Stripping Binary... ")

		local status = os.execute("strip --strip-all "..binary)

		if status/256 == 0 then
			msg.done()
		else
			msg.fail()
		end
	end)
end)

