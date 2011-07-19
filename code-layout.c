#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <qr/bitmap.h>
#include <qr/code.h>
#include <qr/common.h>
#include <qr/layout.h>
#include "constants.h"

struct qr_iterator {
        struct qr_code * code;
        int dim;
        int column;
        int row;
        int up;
        int mask;
        unsigned char * p;
};

void qr_layout_init_mask(struct qr_code * code)
{
        size_t x, y;
        size_t dim = qr_code_width(code);
        struct qr_bitmap * bmp = code->modules;
        const int * am_pos = QR_ALIGNMENT_LOCATION[code->version - 1];
        size_t am_side;

        if (!bmp->mask)
                qr_bitmap_add_mask(bmp);

        assert(bmp->mask);

        memset(bmp->mask, 0, bmp->height * bmp->stride);

        /* slow and stupid, but I'm sleepy */
        for (y = 0; y < bmp->height; ++y) {
                unsigned char * row = bmp->mask + y * bmp->stride;
                for (x = 0; x < bmp->width; ++x) {
                        unsigned char bit = 1 << (x % CHAR_BIT);
                        int off = x / CHAR_BIT;

                        if (x == 6 || y == 6) /* timing */
                                continue;

                        if (x < 9 && y < 9) /* top-left */
                                continue;

                        if (x >= dim - 8 && y < 9) /* top-right */
                                continue;

                        if (x < 9 && y >= dim - 8) /* bottom-left */
                                continue;

                        /* version info */
                        if (code->version >= 7) {
                                if (y < 6 && x >= dim - 11)
                                        continue;
                                if (x < 6 && y >= dim - 11)
                                        continue;
                        }

                        row[off] |= bit;
                }
        }

        /* Alignment pattern */
        am_side = code->version > 1 ? (code->version / 7) + 2 : 0;
        for (y = 0; y < am_side; ++y) {
                for (x = 0; x < am_side; ++x) {
                        int i, j;

                        if ((x == 0 && y == 0) ||
                            (x == 0 && y == am_side - 1) ||
                            (x == am_side - 1 && y == 0))
                                continue;

                        for (j = -2; j <= 2; ++j) {
                                unsigned char * row = bmp->mask + (am_pos[y]+j) * bmp->stride;
                                for (i = -2; i <= 2; ++i) {
                                        unsigned char bit = 1 << ((am_pos[x]+i) % CHAR_BIT);
                                        int off = (am_pos[x]+i) / CHAR_BIT;

                                        row[off] &= ~bit;
                                }
                        }
                }
        }
}

static int is_data_bit(const struct qr_iterator * i)
{
        unsigned char bit = 1 << (i->column % CHAR_BIT);
        int off = (i->row * i->code->modules->stride) + (i->column / CHAR_BIT);

        return i->code->modules->mask[off] & bit;
}

static void set_pointer(struct qr_iterator * i)
{
        i->mask = 1 << (i->column % CHAR_BIT);
        i->p = i->code->modules->bits
                + i->code->modules->stride * i->row
                + i->column / CHAR_BIT;
}

static void advance(struct qr_iterator * i)
{
        do {
                /* This XOR is to account for the vertical strip of
                 * timing bits in column 6 which displaces everything.
                 */
                if ((i->column < 6) ^ !(i->column % 2)) {
                        /* Right-hand part or at left edge */
                        i->column -= 1;
                } else {
                        /* Left-hand part */
                        i->column += 1;

                        if (( i->up && i->row == 0) ||
                            (!i->up && i->row == i->dim - 1)) {
                                /* Hit the top / bottom */
                                i->column -= 2;
                                i->up = !i->up;
                        } else {
                                i->row += i->up ? -1 : 1;
                        }
                }

                if (i->column < 0)
                        continue; /* don't go off left edge */

                /* Check for one-past-end */
                if (i->column == 0 && i->row >= i->dim - 8)
                        break;

        } while (!is_data_bit(i));

        set_pointer(i);
}

struct qr_iterator * qr_layout_begin(struct qr_code * code)
{
        struct qr_iterator * i;

        i = malloc(sizeof(*i));
        if (i) {
                i->dim = qr_code_width(code);
                i->code = code;
                i->column = i->dim - 1;
                i->row = i->dim - 1;
                i->up = 1;
                set_pointer(i);
        }

        return i;
}

void qr_layout_end(struct qr_iterator * i)
{
        free(i);
}

unsigned int qr_layout_read(struct qr_iterator * i)
{
        unsigned int x = 0;
        int b;

        for (b = 0; b < QR_WORD_BITS; ++b) {
                x = (x << 1) | ((*i->p & i->mask) ? 1 : 0);
                advance(i);
        }

        return x;
}

void qr_layout_write(struct qr_iterator * i, unsigned int x)
{
        int b;

        for (b = 0; b < QR_WORD_BITS; ++b) {
                *i->p |= (x & 0x80) ? i->mask : 0;
                advance(i);
                x <<= 1;
        }
}

