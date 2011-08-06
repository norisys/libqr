#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <qr/bitmap.h>

struct qr_bitmap * qr_bitmap_create(size_t width, size_t height, int masked)
{
        struct qr_bitmap * out;
        size_t size;

        out = malloc(sizeof(*out));
        if (!out)
                goto fail;

        out->width = width;
        out->height = height;
        out->stride = (width / CHAR_BIT) + (width % CHAR_BIT ? 1 : 0);

        size = out->stride * height;

        out->mask = 0;
        out->bits = malloc(size);
        if (!out->bits)
                goto fail;
        memset(out->bits, 0, size);

        if (masked) {
                out->mask = malloc(out->stride * width);
                if (!out->mask)
                        goto fail;
                memset(out->mask, 0xFF, size);
        }

        return out;

fail:
        qr_bitmap_destroy(out);
        return 0;
}

void qr_bitmap_destroy(struct qr_bitmap * bmp)
{
        if (bmp) {
                free(bmp->bits);
                free(bmp->mask);
                free(bmp);
        }
}

int qr_bitmap_add_mask(struct qr_bitmap * bmp)
{
        size_t size = bmp->stride * bmp->width;
        bmp->mask = malloc(size);
        if (!bmp->mask)
                return -1;
        memset(bmp->mask, 0xFF, size);
        return 0;
}

struct qr_bitmap * qr_bitmap_clone(const struct qr_bitmap * src)
{
        struct qr_bitmap * bmp;
        size_t size;

        bmp = qr_bitmap_create(src->width, src->height, !!src->mask);
        if (!bmp)
                return 0;

        assert(bmp->stride == src->stride);

        size = bmp->width * bmp->stride;
        memcpy(bmp->bits, src->bits, size);
        if (bmp->mask)
                memcpy(bmp->mask, src->mask, size);

        return bmp;
}

void qr_bitmap_merge(struct qr_bitmap * dest, const struct qr_bitmap * src)
{
        unsigned char * d, * s, * m;
        size_t n;

        assert(dest->stride == src->stride);
        assert(dest->height == src->height);
        assert(src->mask);

        n = dest->stride * dest->height;
        d = dest->bits;
        s = src->bits;
        m = src->mask;

        while (n--) {
                *d   &= ~*m;
                *d++ |= *s++ & *m++;
        }
}

/* CHAR_BIT | mod_bits  (multi-byte) */
static void render_line_1(unsigned char *       out,
                          const unsigned char * in,
                          size_t                mod_bits,
                          size_t                dim,
                          unsigned long         mark,
                          unsigned long         space)
{
        unsigned char in_mask;
        size_t n, b;

        in_mask = 1;
        n = dim;

        while (n-- > 0) {
                unsigned long v = (*in & in_mask) ? mark : space;

                if ((in_mask <<= 1) == 0) {
                        in_mask = 1;
                        ++in;
                }

                b = mod_bits / CHAR_BIT;
                while (b-- > 0) {
                        *out++ = (unsigned char) v;
                        v >>= CHAR_BIT;
                }
        }
}

/* mod_bits | CHAR_BIT  (packed) */
static void render_line_2(unsigned char *       out,
                          const unsigned char * in,
                          size_t                mod_bits,
                          size_t                dim,
                          unsigned long         mark,
                          unsigned long         space)
{
        unsigned char in_mask;
        size_t n, b, step;

        in_mask = 1;
        step = CHAR_BIT / mod_bits;
        n = dim;

        while (n > 0) {
                unsigned char tmp = 0;

                b = step;
                while (b-- > 0) {
                        unsigned long v = (*in & in_mask) ? mark : space;

                        if ((in_mask <<= 1) == 0) {
                                in_mask = 1;
                                ++in;
                        }

                        tmp |= v << (b * mod_bits);
                        if (--n == 0)
                                break;
                };

                *out++ = tmp;
        }
}

void qr_bitmap_render(const struct qr_bitmap * bmp,
                      void *                   buffer,
                      int                      mod_bits,
                      long                     line_stride,
                      int                      line_repeat,
                      unsigned long            mark,
                      unsigned long            space)
{
        unsigned char * out;
        const unsigned char * in;
        size_t n, dim;
        int pack;

        pack = (mod_bits < CHAR_BIT);
        assert(!pack || (CHAR_BIT % mod_bits == 0));
        assert( pack || (mod_bits % CHAR_BIT == 0));

        in = bmp->bits;
        out = buffer;
        dim = bmp->width;

        if (pack) {
                mark &= (1 << mod_bits) - 1;
                space &= (1 << mod_bits) - 1;
        }

        n = dim;
        while (n-- > 0) {
                size_t rpt;
                unsigned char * next;

                if (pack)
                        render_line_2(out, in, mod_bits, dim, mark, space);
                else
                        render_line_1(out, in, mod_bits, dim, mark, space);

                rpt = line_repeat;
                next = out + line_stride;
                while (--rpt > 0) {
                        size_t bits = dim * mod_bits;
                        size_t bytes = bits / CHAR_BIT + !!(bits % CHAR_BIT);
                        memcpy(next, out, bytes);
                        next += line_stride;
                }

                in += bmp->stride;
                out = next;
        }
}

