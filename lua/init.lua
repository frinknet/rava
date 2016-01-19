-- provide instanciated inbuilt libs
gen = require("gen")
opt = require("opt")
msg = require("msg")

rava = require("rava")

for v, f in pairs(rava) do
	_G[v] = f
end
