CC= gcc
CFLAGS= -fPIC -O3 -Wall -ansi -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LDFLAGS= -O3 -Wall -ansi
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -lexpat -licui18n

all: p_abi.so

p_abi.so: $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_abi.o: p_abi.h ../../misc.h

saxHandlers.o: saxHandlers.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf *.so