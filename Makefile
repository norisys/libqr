OBJECTS :=      bitstream.o     \
                code-common.o   \
                code-create.o   \
                code-parse.o    \
                code-render.o   \
                data-common.o   \
                data-create.o   \
                data-parse.o	\
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

