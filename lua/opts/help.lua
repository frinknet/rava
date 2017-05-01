-- Help
opt.add("h", "Show this help dialog", function(r)
	msg.header()

	if _G.DESCRIPTION then
			msg.indent(_G.DESCRIPTION.."\n")
	end

	msg.line("\tUsage:\n")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." [options] [files] [objects]\n")
	msg.line("\tOptions:\n")
	opt.show()
	msg.line()
	msg.line("\tExamples:\n")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." test.lua")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." -e \"print('hello world.')\"")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --binary=appsvr app.lua server.a")
	msg.line("\t"..RAVABIN:gsub("^.*/", "").." --library=dingo main.lua main.lua.o\n")
	msg.line()
	os.exit(r == false)
end)
