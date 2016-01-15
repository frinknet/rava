RAVA_SRC=main.c \
	rava_common.c \
	rava_core.c \
	rava_core_object.c \
	rava_core_serialize.c \
	rava_core_state.c \
	rava_core_stream.c \
	rava_process.c \
	rava_process_cond.c \
	rava_process_fiber.c \
	rava_process_idle.c \
	rava_process_spawn.c \
	rava_process_thread.c \
	rava_process_timer.c \
	rava_socket.c \
	rava_socket_pipe.c \
	rava_socket_tcp.c \
	rava_socket_udp.c \
	rava_system.c \
	rava_system_fs.c \
	rava.c
RAVA_OBJ=$(RAVA_SRC:.c=.o)

CCARGS=-I../libs/luajit/ -I../libs/libuv/ -lpthread

CC := gcc

all: $(RAVA_OBJ)

.c.o:
	@echo CC $<
	@$(CC) -c $(CCARGS) $<

clean:
	@echo CLEAN src
	@rm -f *.o
