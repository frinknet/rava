-- Generate binary data store
opt.add("filestore=name", "Generate a lua data store of binary files", function(name, file, ...)
	if file == nil then
		return opt.run("h")
	end

	if name == true then
		name = file:gsub("%..-$", "")
	end

	gen.filestore(name, file, ...)

	os.exit(0)
end)
