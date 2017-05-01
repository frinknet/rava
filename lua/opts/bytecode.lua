-- Generate bytecode object
opt.add("bytecode=name", "Generate a lua file to bytecode object", function(name, file, fileout, ...)
	if file == nil then
		msg.header()
		msg.indent("Usage: "..RAVABIN:gsub("^.*/", "").." --bytecode [options]\n")
		gen.bytecode()
	end

	if name == true then
		name = file:gsub("%..-$", ""):gsub("/",".")
	end

	if fileout == nil then
		fileout = file:gsub("%..-$", ""):gsub("/",".")
	end

	msg.header()
	msg.format("\nGenerating bytecode %s.o from:\n", file)
	msg.list(file, ...)

	gen.bytecode("-n", name, file, fileout, ...)

	msg.line("\n")

	os.exit(0)
end)
