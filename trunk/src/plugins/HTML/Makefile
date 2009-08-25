CC= gcc
CFLAGS= -fPIC -O3 -Wall -ansi -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
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