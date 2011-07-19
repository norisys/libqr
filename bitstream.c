/**
 * It would perhaps be more sensible just to store the bits
 * as an array of char or similar, but this way is more fun.
 * This is a pretty inefficient implementation, althought I
 * suspect that won't be a problem.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <qr/bitstream.h>

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

struct qr_bitstream {
        size_t pos;    /* bits */
        size_t count;  /* bits */
        size_t bufsiz; /* bytes */
        unsigned char * buffer;
};

static size_t bits_to_bytes(size_t bits)
{
        return (bits / CHAR_BIT) + (bits % CHAR_BIT != 0);
}

static int ensure_available(struct qr_bitstream * stream, size_t bits)
{
        size_t need_bits = stream->pos + bits;
        size_t newsize;

        if (stream->bufsiz * CHAR_BIT >= need_bits)
                return 0;

        newsize = MAX(stream->bufsiz, 100) * CHAR_BIT;
        while (newsize < need_bits)
                newsize *= 2;

        return qr_bitstream_resize(stream, newsize);
}

struct qr_bitstream * qr_bitstream_create(void)
{
        struct qr_bitstream * obj;

        obj = malloc(sizeof(*obj));

        if (obj) {
                obj->pos    = 0;
                obj->count  = 0;
                obj->bufsiz = 0;
                obj->buffer = 0;
        }

        return obj;
}

int qr_bitstream_resize(struct qr_bitstream * stream, size_t bits)
{
        size_t newsize;
        void * newbuf;

        newsize = bits_to_bytes(bits);
        newbuf = realloc(stream->buffer, newsize);

        if (newbuf) {
                stream->bufsiz = newsize;
                stream->buffer = newbuf;
        }

        return newbuf ? 0 : -1;
}

void qr_bitstream_destroy(struct qr_bitstream * stream)
{
        free(stream->buffer);
        free(stream);
}

struct qr_bitstream * qr_bitstream_dup(const struct qr_bitstream * src)
{
        struct qr_bitstream * ret;

        ret = qr_bitstream_create();
        if (!ret)
                return 0;

        if (qr_bitstream_resize(ret, src->count) != 0) {
                free(ret);
                return 0;
        }

        ret->pos   = src->pos;
        ret->count = src->count;
        memcpy(ret->buffer, src->buffer, src->bufsiz);

        return ret;
}

void qr_bitstream_seek(struct qr_bitstream * stream, size_t pos)
{
        assert(pos <= stream->count);
        stream->pos = pos;
}

size_t qr_bitstream_tell(const struct qr_bitstream * stream)
{
        return stream->pos;
}

size_t qr_bitstream_remaining(const struct qr_bitstream * stream)
{
        return stream->count - stream->pos;
}

size_t qr_bitstream_size(const struct qr_bitstream * stream)
{
        return stream->count;
}

unsigned long qr_bitstream_read(struct qr_bitstream * stream, int bits)
{
        unsigned long result = 0;
        unsigned char * byte;
        size_t bitnum;

        assert(qr_bitstream_remaining(stream) >= (size_t) bits);

        byte = stream->buffer + (stream->pos / CHAR_BIT);
        bitnum = stream->pos % CHAR_BIT;

        stream->pos += bits;

        while (bits-- > 0) {
                int bit = (*byte >> bitnum++) & 0x1;
                result = (result << 1) | bit;
                if (bitnum == CHAR_BIT) {
                        bitnum = 0;
                        ++byte;
                }                
        }

        return result;
}

void qr_bitstream_unpack(struct qr_bitstream * stream,
                      unsigned int *     result,
                      size_t             count,
                      int                bitsize)
{
        assert(qr_bitstream_remaining(stream) >= (count * bitsize));

        while (count--)
                *(result++) = qr_bitstream_read(stream, bitsize);
}

int qr_bitstream_write(struct qr_bitstream * stream,
                    unsigned long      value,
                    int                bits)
{
        unsigned char * byte;
        size_t bitnum;

        if (ensure_available(stream, bits) != 0)
                return -1;

        byte = stream->buffer + (stream->pos / CHAR_BIT);
        bitnum = stream->pos % CHAR_BIT;

        stream->pos += bits;
        stream->count = stream->pos; /* truncate */

        while (bits-- > 0) {
                int bit = (value >> bits) & 0x1;
                unsigned char mask = 1 << bitnum++;
                *byte = (*byte & ~mask) | (bit ? mask : 0);
                if (bitnum == CHAR_BIT) {
                        bitnum = 0;
                        ++byte;
                }
        }

        return 0;
}

int qr_bitstream_pack(struct qr_bitstream *   stream,
                   const unsigned int * values,
                   size_t               count,
                   int                  bitsize)
{
        if (ensure_available(stream, count * bitsize) != 0)
                return -1;

        while (count--)
                qr_bitstream_write(stream, *(values++), bitsize);

        return 0;
}

int qr_bitstream_cat(struct qr_bitstream * dest, const struct qr_bitstream * src)
{
        size_t count = qr_bitstream_size(src);
        size_t srcpos;

        if (ensure_available(dest, count) != 0)
                return -1;

        srcpos = qr_bitstream_tell(src);
        qr_bitstream_seek((struct qr_bitstream *)src, 0);
        qr_bitstream_copy(dest, (struct qr_bitstream *)src, count);
        qr_bitstream_seek((struct qr_bitstream *)src, srcpos);

        return 0;
}

int qr_bitstream_copy(struct qr_bitstream * dest,
                      struct qr_bitstream * src,
                      size_t count)
{
        if (qr_bitstream_remaining(src) < count)
                return -1;
        if (ensure_available(dest, count) != 0)
                return -1;

        /* uint must be at least 16 bits */
        for (; count >= 16; count -= 16)
                qr_bitstream_write(
                        dest,
                        qr_bitstream_read((struct qr_bitstream *)src, 16),
                        16);

        if (count > 0)
                qr_bitstream_write(
                        dest,
                        qr_bitstream_read((struct qr_bitstream *)src, count),
                        count);

        return 0;
}

