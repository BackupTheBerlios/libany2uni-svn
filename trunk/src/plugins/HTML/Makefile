CC= gcc
CFLAGS= -O3 -Wall -ansi
LDFLAGS= -O3 -Wall -ansi
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n

all: p_html.so

p_html.so: $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_html.o: p_html.h

htmlReader.o: p_html.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf *.so