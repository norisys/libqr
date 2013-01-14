OBJECTS :=      bitmap.o                \
                bitstream.o             \
                constants.o             \
                code-common.o           \
                code-create.o           \
                code-layout.o           \
                code-parse.o            \
                data-common.o           \
                data-create.o           \
                data-parse.o            \
                galois.o

CFLAGS := -std=c89 -pedantic -I. -Wall
CFLAGS += -g
#CFLAGS += -O3 -DNDEBUG

all : libqr qrgen qrparse

$(OBJECTS) : $(wildcard *.h qr/*.h)

libqr : libqr.a($(OBJECTS))

qrgen : libqr qrgen.c
	$(CC) $(CFLAGS) -o qrgen qrgen.c libqr.a $(shell pkg-config libpng --cflags --libs)

qrparse : libqr qrparse.c
	$(CC) $(CFLAGS) -o qrparse qrparse.c libqr.a

.PHONY : clean
clean:
	$(RM) qr/*~ *~ *.o *.a *.so qrgen

