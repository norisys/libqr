OBJECTS :=      bitstream.o     \
                code-common.o   \
                code-create.o   \
                code-layout.o   \
                code-parse.o    \
                data-common.o   \
                data-create.o   \
                data-parse.o	\
                qr-bitmap.o             \
                qr-bitmap-pbm.o         \
                qr-bitmap-render.o      \
                rs-encode.o

CFLAGS := -std=c89 -pedantic -I. -Wall
CFLAGS += -g
#CFLAGS += -O3 -DNDEBUG

all : libqr test

$(OBJECTS) : $(wildcard *.h qr/*.h)

libqr : libqr.a($(OBJECTS))

test : libqr.a test.c

.PHONY : clean
clean:
	$(RM) qr/*~ *~ *.o *.a *.so test

