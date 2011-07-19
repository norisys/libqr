#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include <qr/bitmap.h>
#include <qr/bitstream.h>
#include <qr/code.h>
#include <qr/common.h>
#include "constants.h"

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

void qr_get_rs_block_sizes(int version,
                           enum qr_ec_level ec,
                           int block_count[2],
                           int data_length[2],
                           int ec_length[2])
{
        /* FIXME: rather than using a big static table, better to
         * calculate these values.
         */
        int total_words = qr_code_total_capacity(version) / 8;
        int data_words = QR_DATA_WORD_COUNT[version - 1][ec ^ 0x1];
        int ec_words = total_words - data_words;
        int total_blocks;

        block_count[0] = QR_RS_BLOCK_COUNT[version - 1][ec ^ 0x1][0];
        block_count[1] = QR_RS_BLOCK_COUNT[version - 1][ec ^ 0x1][1];
        total_blocks = block_count[0] + block_count[1];

        data_length[0] = data_words / total_blocks;
        data_length[1] = data_length[0] + 1;

        assert(data_length[0] * block_count[0] +
               data_length[1] * block_count[1] == data_words);

        ec_length[0] = ec_length[1] = ec_words / total_blocks;

        assert(ec_length[0] * block_count[0] +
               ec_length[1] * block_count[1] == ec_words);
        assert(data_words + ec_words == total_words);
}

void qr_mask_apply(struct qr_bitmap * bmp, int mask)
{
        size_t i, j;

        assert((mask & 0x7) == mask);
        mask &= 0x7;

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
}

