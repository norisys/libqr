#include <limits.h>
#include <stdlib.h>
#include "code-common.h"
#include "code-layout.h"

struct qr_iterator {
        struct qr_code * code;
        int dim;
        int column;
        int row;
        int up;
        int mask;
        unsigned char * p;
};

static int is_data_bit(const struct qr_iterator * i)
{
        if (i->row == 6 || i->column == 6) /* timing */
                return 0;

        if (i->column < 9 && i->row < 9) /* top-left */
                return 0;

        if (i->column >= i->dim - 8 && i->row < 9) /* top-right */
                return 0;

        if (i->column < 9 && i->row >= i->dim - 8) /* bottom-left */
                return 0;

        /* XXX: format data */
        /* XXX: alignment pattern */

        return 1;
}

static void set_pointer(struct qr_iterator * i)
{
        i->mask = 1 << (i->column % CHAR_BIT);
        i->p = i->code->modules
                + i->code->line_stride * i->row
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
                i->dim = code_side_length(code->format);
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

        for (b = 0; b < 8; ++b) {
                x |= (*i->p & i->mask) ? 1 : 0;
                advance(i);
                x <<= 1;
        }

        return x;
}

void qr_layout_write(struct qr_iterator * i, unsigned int x)
{
        int b;

        for (b = 0; b < 8; ++b) {
                *i->p |= (x & 0x80) ? i->mask : 0;
                advance(i);
                x <<= 1;
        }
}

