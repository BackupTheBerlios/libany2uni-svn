CC= gcc
CFLAGS= -fPIC -O3 -Wall -ansi -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LDFLAGS= -O3 -Wall -ansi
EXEC= p_powerpoint.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_powerpoint.o: p_powerpoint.h

ppt_utils.o: p_powerpoint.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf *.so