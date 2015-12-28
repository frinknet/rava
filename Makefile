all: rava

rava: libs/luajit src/rava.lua modules/*.lua
	libs/luajit src/rava.lua -csmr --compile=rava src/rava.lua

clean:
	rm src/*.o modules/*.o rava

install:
	cp rava /usr/local/bin/
