CC= gcc
LDFLAGS= -O3 -Wall -ansi
CFLAGS= -O3 -Wall -ansi -fno-strict-aliasing
EXEC= libany2uni.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n -ldl -lz
PLUGINSDIR = plugins/abiword plugins/HTML plugins/koffice plugins/latex \
	plugins/openoffice plugins/pdf plugins/scribus plugins/word plugins/XML

all: $(EXEC)

$(EXEC): $(OBJ)
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && make ; cd ../.. \
	; done
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

test: $(OBJ)
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && make ; cd ../.. \
	; done
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

userInterface.o: userInterface.h misc.h

fileMgr.o: fileMgr.h misc.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

install:
	mkdir -p /usr/lib/libany2uni
	mkdir -p /usr/include/libany2uni
	for DIR in $(PLUGINSDIR); do \
		cp $$DIR/*.so  /usr/lib/libany2uni/ \
	; done
	cp $(EXEC) /usr/lib/
	cp *.h /usr/include/libany2uni/
	cp plugins/pdf/glyphlist.txt /usr/lib/libany2uni/

clean:
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && make clean ; cd ../.. \
	; done
	rm -rf *.o

distclean : clean
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && make distclean ; cd ../.. \
	; done
	rm -rf test *.so