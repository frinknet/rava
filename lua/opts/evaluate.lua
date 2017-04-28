-- Run lua from commandline
opt.add("e", "Evaluates a string lua code", function(flag, ...)
	gen.eval(...)
	os.exit(0)
end)

