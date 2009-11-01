OBJECTS :=      capacity.o              \
                code-common.o           \
                code-create.o           \
                code-layout.o           \
                code-parse.o            \
                data-common.o           \
                data-create.o           \
                data-parse.o            \
                qr-bitmap.o             \
                qr-bitmap-render.o      \
                qr-bitstream.o          \
                qr-mask.o               \
                rs-encode.o

CFLAGS := -std=c89 -pedantic -I. -Wall
CFLAGS += -g
#CFLAGS += -O3 -DNDEBUG

all : libqr qrgen

$(OBJECTS) : $(wildcard *.h qr/*.h)

libqr : libqr.a($(OBJECTS))

qrgen : libqr qrgen.c
	$(CC) $(CFLAGS) -o qrgen qrgen.c libqr.a

.PHONY : clean
clean:
	$(RM) qr/*~ *~ *.o *.a *.so qrgen

