CC= gcc
LDFLAGS= -O3 -Wall -ansi
CFLAGS= -O3 -Wall -ansi -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
EXEC= any2utf8
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -lany2uni -licui18n

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf $(EXEC)