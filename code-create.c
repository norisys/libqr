#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <qr/code.h>

#include "code-common.h"
#include "code-layout.h"
#include "data-common.h"
#include "rs.h"

#define MIN(a, b) ((b) < (a) ? (b) : (a))

#include <stdio.h>
static void x_dump(struct qr_bitstream * bits)
{
        size_t i, n;

        qr_bitstream_seek(bits, 0);
        n = qr_bitstream_size(bits);
        for (i = 0; i < n; ++i) {
                printf("%d", qr_bitstream_read(bits, 1));
                if (i % 8 == 7)
                        printf(" ");
                if ((i+1) % (7 * 8) == 0)
                        printf("\n");
        }
        printf("\n");
}

static void setpx(struct qr_code * code, int x, int y)
{
        size_t off = y * code->modules->stride + x / CHAR_BIT;
        unsigned char bit = 1 << (x % CHAR_BIT);

        code->modules->bits[off] |= bit;
}

static void draw_locator(struct qr_code * code, int x, int y)
{
        int i;
        for (i = 0; i < 6; ++i) {
                setpx(code, x + i, y + 0);
                setpx(code, x + 6, y + i);
                setpx(code, x + i + 1, y + 6);
                setpx(code, x, y + i + 1);
        }
        for (i = 0; i < 9; ++i)
                setpx(code, x + 2 + i % 3, y + 2 + i / 3);
}

static void draw_fixed_bits(struct qr_code * code)
{
        int dim = code_side_length(code->format);
        int i;

        /* Locator pattern */
        draw_locator(code, 0, 0);
        draw_locator(code, 0, dim - 7);
        draw_locator(code, dim - 7, 0);

        /* Timing pattern */
        for (i = 8; i < dim - 8; i += 2) {
                setpx(code, i, 6);
                setpx(code, 6, i);
        }

        /* XXX: alignment pattern */
}

static int pad_data(struct qr_bitstream * bits, size_t limit)
{
        /* This function is not very nice. Sorry. */

        size_t count, n;

        assert(qr_bitstream_size(bits) <= limit);

        if (qr_bitstream_resize(bits, limit) != 0)
                return -1;

        n = qr_bitstream_size(bits);
        qr_bitstream_seek(bits, n);
        count = limit - n;

        /* First append the terminator (0000) if possible,
         * and pad with zeros up to an 8-bit boundary
         */
        n = (n + 4) % 8;
        if (n != 0)
                n = 8 - n;
        n = MIN(count, n + 4);
        qr_bitstream_write(bits, 0, n);
        count -= n;

        assert(count % 8 == 0); /* since data codewords are 8 bits */

        /* Finally pad with the repeating sequence 11101100 00010001 */
        while (count >= 16) {
                qr_bitstream_write(bits, 0xEC11, 16);
                count -= 16;
        }
        if (count > 0) {
                assert(count == 8);
                qr_bitstream_write(bits, 0xEC, 8);
        }

        return 0;
}

static struct qr_bitstream * make_data(int                format,
                                    enum qr_ec_level   ec,
                                    struct qr_bitstream * data)
{
        const size_t total_bits = code_total_capacity(format);
        const size_t total_words = total_bits / 8;
        size_t block_count, data_words, rs_words;
        size_t i;
        struct qr_bitstream * dcopy = 0;
        struct qr_bitstream * out = 0;
        struct qr_bitstream ** blocks = 0;

        /* Set up the output stream */
        out = qr_bitstream_create();
        if (!out)
                return 0;

        if (qr_bitstream_resize(out, total_bits) != 0)
                goto fail;

        /**
         * XXX: For our test case (1-M) there is only one RS block.
         * This is not the case for most other formats, so we'll
         * have to deal with this eventually.
         */
        block_count = 1;
        data_words = 16;
        rs_words = 10;
        assert(data_words + rs_words == total_words);

        /* Make a copy of the data and pad it */
        dcopy = qr_bitstream_copy(data);
        if (!dcopy)
                goto fail;

        if (pad_data(dcopy, data_words * 8) != 0)
                goto fail;

        puts("Pad data:");
        x_dump(dcopy);

        /* Make space for the RS blocks */
        blocks = calloc(block_count, sizeof(*blocks));
        if (!blocks)
                goto fail;

        /* Generate RS codewords */
        qr_bitstream_seek(dcopy, 0);
        puts("Generate RS blocks:");
        for (i = 0; i < block_count; ++i) {
                /* XXX: some blocks may be longer */
                blocks[i] = rs_generate_words(dcopy, data_words, rs_words);
                if (!blocks[i]) {
                        while (i--)
                                qr_bitstream_destroy(blocks[i]);
                        free(blocks);
                        blocks = 0;
                        goto fail;
                }
                x_dump(blocks[i]);
        }

        /* Finally, write everything out in the correct order */
        /* XXX: need to handle multiple blocks */
        qr_bitstream_cat(out, dcopy);
        qr_bitstream_cat(out, blocks[0]);
        qr_bitstream_write(out, 0, total_bits - total_words * 8);

        puts("Final bitstream:");
        x_dump(out);
exit:
        if (blocks) {
                while (block_count--)
                       qr_bitstream_destroy(blocks[block_count]);
                free(blocks);
        }
        if (dcopy)
                qr_bitstream_destroy(dcopy);

        return out;

fail:
        qr_bitstream_destroy(out);
        out = 0;
        goto exit;
}

struct qr_code * qr_code_create(enum qr_ec_level       ec,
                                const struct qr_data * data)
{
        struct qr_code * code;
        struct qr_bitstream * bits = 0;
        struct qr_iterator * layout;
        size_t dim;

        code = malloc(sizeof(*code));
        if (!code)
                return 0;

        dim = code_side_length(data->format);

        code->format = data->format;
        code->modules = qr_bitmap_create(dim, dim, 0);

        if (!code->modules)
                goto fail;

        draw_fixed_bits(code);

        bits = make_data(data->format, ec, data->bits);
        if (!bits)
                goto fail;

        layout = qr_layout_begin(code);
        if (!layout)
                goto fail;

        qr_bitstream_seek(bits, 0);
        while (qr_bitstream_remaining(bits) >= 8)
                qr_layout_write(layout, qr_bitstream_read(bits, 8));
        qr_layout_end(layout);

exit:
        if (bits)
                qr_bitstream_destroy(bits);

        return code;

fail:
        qr_code_destroy(code);
        code = 0;
        goto exit;
}

