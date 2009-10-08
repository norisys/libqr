#include <string.h>
#include <limits.h>
#include <assert.h>

#include <qr/bitmap.h>

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
        size_t n, b, step, shift;

        in_mask = 1;
        step = CHAR_BIT / mod_bits;
        shift = CHAR_BIT - mod_bits;
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

                        tmp = (tmp >> mod_bits) | (v << shift);
                        if (--n == 0) {
                                tmp >>= b * mod_bits;
                                break;
                        }
                };

                *out++ = tmp;
        }
}

void qr_bitmap_render(const struct qr_bitmap * bmp,
                      void *                   buffer,
                      size_t                   mod_bits,
                      size_t                   line_stride,
                      size_t                   line_repeat,
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
                while (rpt-- > 0) {
                        memcpy(next, out, line_stride);
                        next += line_stride;
                }

                in += bmp->stride;
                out = next;
        }
}

