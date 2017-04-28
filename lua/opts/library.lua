-- Build library
opt.add("library=name", "Build a lua library from files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	name = name:gsub("%.[ao]$", ""):gsub("%.lua$", "")

	msg.header()
	msg.format("Building %s.a library from:\n\n", name)
	msg.list(file, ...)
	gen.build(name, file, ...)
	msg.line("\n\n")

	os.exit(0)
end)
