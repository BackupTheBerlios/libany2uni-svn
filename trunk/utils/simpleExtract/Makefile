CC= gcc
LDFLAGS= -O3 -Wall -ansi
CFLAGS= -O3 -Wall -ansi
EXEC= simpleExtract
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= ../../src/libany2uni.so -licui18n

all: $(EXEC)

simpleExtract: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
