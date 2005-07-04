CC= gcc
LDFLAGS= -O3 -Wall -ansi
CFLAGS= -O3 -Wall -ansi -fno-strict-aliasing  -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -DINSTALL_PATH="\"$(libdir)\"" -DINCLUDE_PATH="\"$(includedir)\""
EXEC= libtkz.so
EXEC= libany2uni.so
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
LIBS= -licui18n -ldl -lz
prefix= /usr/local
libdir= $(prefix)/lib
includedir= $(prefix)/include
PLUGINSDIR = plugins/abiword plugins/HTML plugins/koffice plugins/latex \
	plugins/openoffice plugins/pdf plugins/scribus plugins/word \
	plugins/XML plugins/rtf plugins/excel plugins/powerpoint \
	plugins/mht plugins/txt

all: $(EXEC)

$(EXEC): $(OBJ)
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && $(MAKE) libdir=$(libdir) ; cd ../.. \
	; done
	$(CC) -o $@ -shared $^ $(LDFLAGS) $(LIBS)

test: $(OBJ)
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && $(MAKE) libdir=$(libdir) ; cd ../.. \
	; done
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

userInterface.o: userInterface.h misc.h

internals.o: internals.h misc.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

install:
	mkdir -p $(libdir)/libany2uni
	mkdir -p $(includedir)/libany2uni
	for DIR in $(PLUGINSDIR); do \
		cp $$DIR/*.so  $(libdir)/libany2uni/ \
	; done
	cp $(EXEC) $(libdir)/
	cp *.h $(includedir)/libany2uni/
	cp plugins/pdf/glyphlist.txt $(libdir)/libany2uni/

clean:
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && $(MAKE) clean ; cd ../.. \
	; done
	rm -rf *.o

distclean : clean
	@for DIR in $(PLUGINSDIR); do \
		cd $$DIR  && $(MAKE) distclean ; cd ../.. \
	; done
	rm -rf test *.so