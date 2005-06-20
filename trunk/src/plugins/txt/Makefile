CC= gcc
CFLAGS= -O3 -Wall -ansi
LDFLAGS= -O3 -Wall -ansi
EXEC= p_txt.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_txt.o: p_txt.h

txtUtils.o: p_txt.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf *.so