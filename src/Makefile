RAVA_SRC=main.c rava.c ray.c
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
