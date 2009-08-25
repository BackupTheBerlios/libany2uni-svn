CC= gcc
CFLAGS= -fPIC -O3 -Wall -ansi -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LDFLAGS= -O3 -Wall -ansi
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -lexpat -licui18n

all: p_xml.so

p_xml.so: $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_xml.o: p_xml.h

saxHandlers.o: saxHandlers.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf *.so