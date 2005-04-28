CC= gcc
LDFLAGS= -O3 -Wall -ansi
CFLAGS= -O3 -Wall -ansi -fno-strict-aliasing
EXEC= libany2uni.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n -ldl -lz

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

test: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

userInterface.o: userInterface.h misc.h

fileMgr.o: fileMgr.h misc.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

distclean : clean
	rm -rf test *.so