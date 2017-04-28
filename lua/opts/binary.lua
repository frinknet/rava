-- Compile binary
opt.add("binary=name", "Compile a binary from lua files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	msg.header()
	msg.format("Compiling %s from:\n\n", name)
	msg.list(...)
	gen.compile(name, file, ...)
	msg.line("\n\n")

	os.exit(0)
end)
