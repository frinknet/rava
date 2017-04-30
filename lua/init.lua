-- provide instanciated inbuilt libs
opt = require("opt")
msg = require("msg")
rava = require("rava")

for v, f in pairs(rava) do
	_G[v] = f
end
