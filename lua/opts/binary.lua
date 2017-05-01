-- Compile binary
opt.add("binary=name", "Compile a binary from lua files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	msg.header()
	msg.format("\nCompiling %s from:\n", name)
	msg.list(file, ...)

	gen.compile(name, file, ...)

	msg.line("\n")

	os.exit(0)
end)
