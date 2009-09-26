#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <qr/code.h>

#include "code-common.h"
#include "data-common.h"
#include "rs.h"

#define MIN(a, b) ((b) < (a) ? (b) : (a))

#include <stdio.h>
static void x_dump(struct bitstream * bits)
{
        size_t i, n;

        bitstream_seek(bits, 0);
        n = bitstream_size(bits);
        for (i = 0; i < n; ++i) {
                printf("%d", bitstream_read(bits, 1));
                if (i % 8 == 7)
                        printf(" ");
                if ((i+1) % (7 * 8) == 0)
                        printf("\n");
        }
        printf("\n");
}

static int pad_data(struct bitstream * bits, size_t limit)
{
        /* This function is not very nice. Sorry. */

        size_t count, n;

        assert(bitstream_size(bits) <= limit);

        if (bitstream_resize(bits, limit) != 0)
                return -1;

        n = bitstream_size(bits);
        bitstream_seek(bits, n);
        count = limit - n;

        /* First append the terminator (0000) if possible,
         * and pad with zeros up to an 8-bit boundary
         */
        n = (n + 4) % 8;
        if (n != 0)
                n = 8 - n;
        n = MIN(count, n + 4);
        bitstream_write(bits, 0, n);
        count -= n;

        assert(count % 8 == 0); /* since data codewords are 8 bits */

        /* Finally pad with the repeating sequence 11101100 00010001 */
        while (count >= 16) {
                bitstream_write(bits, 0xEC11, 16);
                count -= 16;
        }
        if (count > 0) {
                assert(count == 8);
                bitstream_write(bits, 0xEC, 8);
        }

        return 0;
}

static struct bitstream * make_data(int                format,
                                    enum qr_ec_level   ec,
                                    struct bitstream * data)
{
        const size_t total_bits = code_total_capacity(format);
        const size_t total_words = total_bits / 8;
        size_t block_count, data_words, rs_words;
        size_t i;
        struct bitstream * dcopy = 0;
        struct bitstream * out = 0;
        struct bitstream ** blocks = 0;

        /* Set up the output stream */
        out = bitstream_create();
        if (!out)
                return 0;

        if (bitstream_resize(out, total_bits) != 0)
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
        dcopy = bitstream_copy(data);
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
        bitstream_seek(dcopy, 0);
        puts("Generate RS blocks:");
        for (i = 0; i < block_count; ++i) {
                /* XXX: some blocks may be longer */
                blocks[i] = rs_generate_words(dcopy, data_words, rs_words);
                if (!blocks[i]) {
                        while (i--)
                                bitstream_destroy(blocks[i]);
                        free(blocks);
                        blocks = 0;
                        goto fail;
                }
                x_dump(blocks[i]);
        }

        /* Finally, write everything out in the correct order */
        /* XXX: need to handle multiple blocks */
        bitstream_cat(out, dcopy);
        bitstream_cat(out, blocks[0]);
        bitstream_write(out, 0, total_bits - total_words * 8);

        puts("Final bitstream:");
        x_dump(out);
exit:
        if (blocks) {
                while (block_count--)
                       bitstream_destroy(blocks[block_count]);
                free(blocks);
        }
        if (dcopy)
                bitstream_destroy(dcopy);

        return out;

fail:
        bitstream_destroy(out);
        out = 0;
        goto exit;
}

struct qr_code * qr_code_create(enum qr_ec_level       ec,
                                const struct qr_data * data)
{
        struct qr_code * code;
        struct bitstream * ecdata;

        code = malloc(sizeof(*code));
        if (!code)
                return 0;

        code->format = data->format;
        ecdata = make_data(data->format, ec, data->bits);

        if (!ecdata) {
                free(code);
                return 0;
        }

        /* TODO: allocate bitmap; layout */

        return 0;
}

