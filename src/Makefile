RAVA_SRC=\
	rava_core_object.c \
	rava_core_serialize.c \
	rava_core_state.c \
	rava_core_stream.c \
	rava_core.c \
	rava_fs.c \
	rava_process_cond.c \
	rava_process_fiber.c \
	rava_process_idle.c \
	rava_process_spawn.c \
	rava_process_thread.c \
	rava_process_timer.c \
	rava_process.c \
	rava_socket_pipe.c \
	rava_socket_tcp.c \
	rava_socket_udp.c \
	rava_socket.c \
	rava_system.c \
	rava.c
RAVA_OBJ=$(RAVA_SRC:.c=.o)
RAVA_GEN=librava.a rava.so ravamain.a
RAVA_INC=../libs/luajit/libluajit.a ../libs/libuv/libuv.a

CCARGS=-I../libs/luajit/ -I../libs/libuv/ -pthread -ldl -lm

CC := gcc
LD := ld

all: $(RAVA_GEN) qmain.o

ravamain.a: main.o librava.a
	@echo LD $+ -o $@
	@$(LD) -r -static $+ -o $@

rava.so: librava.a
	@echo CC $+ -o $@
	@$(CC) -shared $(CCARGS) $+ -o $@

librava.a: $(RAVA_OBJ) $(RAVA_INC)
	@echo LD $+ -o $@
	@$(LD) -r -static $+ -o $@

.c.o:
	@echo CC $< -o $@
	@$(CC) -fPIC -c $(CCARGS) $<

clean:
	@echo RM main.o $(RAVA_OBJ) $(RAVA_GEN)
	@rm -f main.o $(RAVA_OBJ) $(RAVA_GEN)
