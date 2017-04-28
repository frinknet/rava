-- No output, except Errors..
opt.add("q", "Quiet except for errors", function()
	function msg.warning(...) end
	function msg.info(...) end
	function msg.done(...) end
	function msg.fail(...) end
	function msg.line(...) end
	function print(...) end
end)

-- No output at all.
opt.add("Q", "Quelch everything including error", function()
	function msg.add() end
	function print(...) end
end)


