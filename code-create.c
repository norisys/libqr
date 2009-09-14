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

static int add_ecc(struct bitstream * bits, int format, enum qr_ec_level ec)
{
        puts("Before ecc:");
        x_dump(bits);
        {
                const int g[10] = { 251, 67, 61, 118, 70, 64, 94, 32, 45 };
                int rs_words = 10; /* 1-M */
                struct bitstream * rs;

                rs = rs_generate_words(rs_words, g, bits);
                puts("ecc part:");
                x_dump(rs);
        }
        return -1;
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
        size_t data_bits = 16 * 8 /*XXX*/;
        struct bitstream * out;

        out = bitstream_copy(data);
        if (!out)
                return 0;

        if (pad_data(out, data_bits) != 0)
                goto fail;

        if (add_ecc(out, format, ec) != 0)
                goto fail;

        return out;
fail:
        bitstream_destroy(out);
        return 0;
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

