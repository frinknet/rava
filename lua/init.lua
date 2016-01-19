-- provide instanciated inbuilt libs
gen = require("gen")
opt = require("opt")
msg = require("msg")

fs      = require("rava.fs")
process = require("rava.process")
socket  = require("rava.socket")
system  = require("rava.system")

-- start the main file
require("main")
