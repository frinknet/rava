-- Generate binary data store
opt.add("filestore=name", "Generate a lua data store of binary files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	msg.header()
	msg.format("Packing file store %s.o from:\n\n", name)
	msg.list(file, ...)
	msg.line("\n\n")

	gen.filestore(name, file, ...)

	os.exit(0)
end)
