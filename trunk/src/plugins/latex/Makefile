CC= gcc
CFLAGS= -O3 -Wall -ansi
LDFLAGS= -O3 -Wall -ansi
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n

all: p_latex.so

p_latex.so: $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

p_latex.o: p_latex.h latexTags.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
