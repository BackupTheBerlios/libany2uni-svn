CC= gcc
CFLAGS= -Wall -ansi -DINSTALL_PATH="\"$(libdir)\"" -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LDFLAGS= -Wall -ansi
EXEC= p_pdf.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n -lz -lglib-2.0

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_pdf.o: p_pdf.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean: clean
	rm -rf *.so