CC= gcc
CFLAGS= -O3 -Wall -ansi
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
