#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include <qr/code.h>
#include <qr/bitmap.h>
#include <qr/bitstream.h>

void qr_code_destroy(struct qr_code * code)
{
        if (code) {
                qr_bitmap_destroy(code->modules);
                free(code);
        }
}

int qr_code_width(const struct qr_code * code)
{
        return code->version * 4 + 17;
}

size_t qr_code_total_capacity(int version)
{
	int side = version * 4 + 17;

	int alignment_side = version > 1 ? (version / 7) + 2 : 0;

	int alignment_count = alignment_side >= 2 ?
		alignment_side * alignment_side - 3 : 0;

	int locator_bits = 8*8*3;

	int format_bits = 8*4 - 1 + (version >= 7 ? 6*3*2 : 0);

	int timing_bits = 2 * (side - 8*2 -
		(alignment_side > 2 ? (alignment_side - 2) * 5 : 0));

	int function_bits = timing_bits + format_bits + locator_bits
		+ alignment_count * 5*5;

	return side * side - function_bits;
}

struct qr_bitmap * qr_mask_apply(const struct qr_bitmap * orig,
                                 unsigned int mask)
{
        struct qr_bitmap * bmp;
        int i, j;

        if (mask & ~0x7)
                return 0;

        bmp = qr_bitmap_clone(orig);
        if (!bmp)
                return 0;

        /* Slow version for now; we can optimize later */

        for (i = 0; i < bmp->height; ++i) {
                unsigned char * p = bmp->bits + i * bmp->stride;

                for (j = 0; j < bmp->width; ++j) {
                        int bit = j % CHAR_BIT;
                        size_t off = j / CHAR_BIT;
                        int t;

                        switch (mask) {
                        case 0: t = (i + j) % 2; break;
                        case 1: t = i % 2; break;
                        case 2: t = j % 3; break;
                        case 3: t = (i + j) % 3; break;
                        case 4: t = (i/2 + j/3) % 2; break;
                        case 5: t = ((i*j) % 2) + ((i*j) % 3); break;
                        case 6: t = (((i*j) % 2) + ((i*j) % 3)) % 2; break;
                        case 7: t = (((i*j) % 3) + ((i+j) % 2)) % 2; break;
                        }

                        p[off] ^= (t == 0) << bit;
                }
        }

        return bmp;
}

