CC= gcc
CFLAGS= -O3 -Wall -ansi
LDFLAGS= -O3 -Wall -ansi
EXEC= p_pdf.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n -lz

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