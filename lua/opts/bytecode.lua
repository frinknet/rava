-- Generate bytecode object
opt.add("bytecode=name", "Generate a lua file to bytecode object", function(name, file, ...)
	if file == nil then
		msg.format("%s v%s - %s\n", APPNAME, VERSION, jit.version)
		msg.indent("Usage: "..RAVABIN:gsub("^.*/", "").." --bytecode [opt]\n")
		gen.bytecode()
		msg.line()
	end

	if name == true then
		name = file:gsub("%..-$", ""):gsub("/",".")
	end

	gen.bytecode("-n", name, file, ...)

	os.exit(0)
end)
