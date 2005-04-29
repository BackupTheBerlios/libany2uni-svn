CC= gcc
LDFLAGS= -O3 -Wall -ansi
CFLAGS= -O3 -Wall -ansi
EXEC= any2utf8
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= /usr/lib/libany2uni/libany2uni.so -licui18n

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf $(EXEC)