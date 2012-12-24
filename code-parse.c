#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qr/bitmap.h>
#include <qr/bitstream.h>
#include <qr/code.h>
#include <qr/common.h>
#include <qr/layout.h>
#include <qr/parse.h>

#include "constants.h"
#include "galois.h"

/* XXX: duplicated */
static int get_px(const struct qr_bitmap * bmp, int x, int y)
{
        size_t off = y * bmp->stride + x / CHAR_BIT;
        unsigned char bit = 1 << (x % CHAR_BIT);

        return bmp->bits[off] & bit;
}

static int unpack_bits(int version,
                       enum qr_ec_level ec,
                       struct qr_bitstream * raw_bits,
                       struct qr_bitstream * bits_out)
{
        /* FIXME: more comments to explain the algorithm */

        int total_words = qr_code_total_capacity(version) / QR_WORD_BITS;
        int block_count[2], data_length[2], ec_length[2];
        int total_blocks;
        int i, w, block;
        struct qr_bitstream ** blocks = 0;
        int status = -1;

        qr_get_rs_block_sizes(version, ec, block_count, data_length, ec_length);
        total_blocks = block_count[0] + block_count[1];

        status = qr_bitstream_resize(bits_out,
                (block_count[0] * data_length[0] +
                 block_count[1] * data_length[1]) * QR_WORD_BITS);
        if (status != 0)
                goto cleanup;

        blocks = malloc(total_blocks * sizeof(*blocks));
        if (blocks == NULL)
                goto cleanup;

        for (i = 0; i < total_blocks; ++i)
                blocks[i] = NULL;

        /* Allocate temporary space for the blocks */
        for (i = 0; i < total_blocks; ++i) {
                int type = (i >= block_count[0]);
                blocks[i] = qr_bitstream_create();
                if (blocks[i] == NULL)
                        goto cleanup;
                status = qr_bitstream_resize(blocks[i],
                        (data_length[type] + ec_length[type]) * QR_WORD_BITS);
                if (status != 0)
                        goto cleanup;
        }

        /* Read in the data & EC (see spec table 19) */

        qr_bitstream_seek(raw_bits, 0);

        fprintf(stderr, "block counts %d and %d\n", block_count[0], block_count[1]);
        fprintf(stderr, "row lengths %d and %d\n", data_length[0], data_length[1]);

        block = 0;
        for (w = 0; w < total_words; ++w) {
                if (block == 0 && w / total_blocks >= data_length[0] && block_count[1] != 0) {
                        /* Skip the short blocks, if there are any */
                        block += block_count[0];
                }
                qr_bitstream_copy(blocks[block], raw_bits, QR_WORD_BITS);
                block = (block + 1) % total_blocks;
        }

        /* XXX: apply ec */

        for (block = 0; block < total_blocks; ++block) {
                int type = (block >= block_count[0]);
                struct qr_bitstream * stream = blocks[block];
                qr_bitstream_seek(stream, 0);
                qr_bitstream_copy(bits_out, stream, data_length[type] * QR_WORD_BITS);
        }
        status = 0;

cleanup:
        if (blocks != NULL) {
                for (block = 0; block < total_blocks; ++block)
                        if (blocks[block] != NULL)
                                qr_bitstream_destroy(blocks[block]);
                free(blocks);
        }

        return status;
}

static int read_bits(const struct qr_code * code,
                     enum qr_ec_level ec,
                     struct qr_bitstream * data_bits)
{
        const size_t total_bits = qr_code_total_capacity(code->version);
        const size_t total_words = total_bits / QR_WORD_BITS;
        struct qr_bitstream * raw_bits;
        struct qr_iterator * layout;
        size_t w;
        int ret = -1;

        raw_bits = qr_bitstream_create();
        if (raw_bits == NULL)
                goto cleanup;

        ret = qr_bitstream_resize(raw_bits, total_words * QR_WORD_BITS);
        if (ret != 0)
                goto cleanup;

        layout = qr_layout_begin((struct qr_code *) code); /* dropping const! */
        if (layout == NULL)
                goto cleanup;
        for (w = 0; w < total_words; ++w)
                qr_bitstream_write(raw_bits, qr_layout_read(layout), QR_WORD_BITS);
        qr_layout_end(layout);

        ret = unpack_bits(code->version, ec, raw_bits, data_bits);

cleanup:
        if (raw_bits != NULL)
                qr_bitstream_destroy(raw_bits);

        return ret;
}

static int read_format(const struct qr_bitmap * bmp, enum qr_ec_level * ec, int * mask)
{
        int dim;
        int i;
        unsigned bits1, bits2;
        enum qr_ec_level ec1, ec2;
        int mask1, mask2;
        int err1, err2;

        dim = bmp->width;
        bits1 = bits2 = 0;

        for (i = 14; i >= 8; --i) {
                bits1 = (bits1 << 1) | !!get_px(bmp, 14 - i + (i <= 8), 8);
                bits2 = (bits2 << 1) | !!get_px(bmp, 8, dim - 1 - (14 - i));
        }

        for (; i >= 0; --i) {
                bits1 = (bits1 << 1) | !!get_px(bmp, 8, i + (i >= 6));
                bits2 = (bits2 << 1) | !!get_px(bmp, dim - 1 - i, 8);
        }

        fprintf(stderr, "read format bits %x / %x\n", bits1, bits2);

        err1 = qr_decode_format(bits1, &ec1, &mask1);
        err2 = qr_decode_format(bits2, &ec2, &mask2);

        if (err1 < 0 && err2 < 0)
                return -1;

        if (err1 < err2)
                *ec = ec1, *mask = mask1;
        else
                *ec = ec2, *mask = mask2;

        return 0;
}

static int read_version(const struct qr_bitmap * bmp)
{
        int dim;
        int i;
        unsigned long bits1, bits2;
        int ver1, ver2;
        int err1, err2;

        dim = bmp->width;
        bits1 = bits2 = 0;

        for (i = 17; i >= 0; --i) {
                bits1 = (bits1 << 1) | !!get_px(bmp, i / 3, dim - 11 + (i % 3));
                bits2 = (bits2 << 1) | !!get_px(bmp, dim - 11 + (i % 3), i / 3);
        }

        fprintf(stderr, "read version bits %lx / %lx\n", bits1, bits2);

        err1 = qr_decode_version(bits1, &ver1);
        err2 = qr_decode_version(bits2, &ver2);

        fprintf(stderr, "got versions %d[%d] / %d[%d]\n", ver1, err1, ver2, err2);

        if (err1 < 0 && err2 < 0)
                return -1;

        return err1 < err2 ? ver1 : ver2;
}

int qr_code_parse(const void *      buffer,
                  size_t            line_bits,
                  size_t            line_stride,
                  size_t            line_count,
                  struct qr_data ** data)
{
        /* TODO: more informative return values for errors */
        struct qr_bitmap src_bmp;
        struct qr_code code;
        enum qr_ec_level ec;
        int mask;
        struct qr_bitstream * data_bits = NULL;
        int status;

        fprintf(stderr, "parsing code bitmap %lux%lu\n", (unsigned long) line_bits, (unsigned long) line_count);

        if (line_bits != line_count
            || line_bits < 21
            || (line_bits - 17) % 4 != 0) {
                /* Invalid size */
                fprintf(stderr, "Invalid size\n");
                return -1;
        }

        code.version = (line_bits - 17) / 4;
        fprintf(stderr, "assuming version %d\n", code.version);

        src_bmp.bits = (unsigned char *) buffer; /* dropping const! */
        src_bmp.mask = NULL;
        src_bmp.stride = line_stride;
        src_bmp.width = line_bits;
        src_bmp.height = line_count;

        if (code.version >= 7 && read_version(&src_bmp) != code.version) {
                fprintf(stderr, "Invalid version info\n");
                return -1;
        }

        if (read_format(&src_bmp, &ec, &mask) != 0) {
                fprintf(stderr, "Failed to read format\n");
                return -1;
        }

        fprintf(stderr, "detected ec type %d; mask %d\n", ec, mask);

        code.modules = qr_bitmap_clone(&src_bmp);
        if (code.modules == NULL) {
                status = -1;    
                goto cleanup;
        }
        qr_mask_apply(code.modules, mask);

        qr_layout_init_mask(&code);

        data_bits = qr_bitstream_create();
        if (data_bits == NULL) {
                status = -1;
                goto cleanup;
        }

        status = read_bits(&code, ec, data_bits);
        if (status != 0)
                goto cleanup;

        *data = malloc(sizeof(**data));
        if (*data == NULL) {
                status = -1;
                goto cleanup;
        }

        (*data)->version = code.version;
        (*data)->ec = ec;
        (*data)->bits = data_bits;
        (*data)->offset = 0;

        data_bits = 0;
        status = 0;

cleanup:
        if (data_bits)
                qr_bitstream_destroy(data_bits);
        if (code.modules)
                qr_bitmap_destroy(code.modules);

        return status;
}

int qr_decode_format(unsigned long bits, enum qr_ec_level * ec, int * mask)
{
        bits ^= QR_FORMAT_MASK;

        /* TODO: check and fix errors */

        bits >>= 10;
        *mask = bits & 7;
        *ec = bits >> 3;

        return 0;
}

int qr_decode_version(unsigned long bits, int * version)
{
        int v, errors;
        unsigned long version_bits;
        int best_err, best_version;

        if (bits != (bits & 0x3FFFF))
                fprintf(stderr, "WARNING: excess version bits");

        best_err = 18;

        for (v = 7; v <= 40; ++v) {
                /* FIXME: cache these values */
                /* see calc_version_bits() */
                version_bits = v;
                version_bits <<= 12;
                version_bits |= gf_residue(version_bits, QR_VERSION_POLY);

                /* count errors */
                errors = 0;
                version_bits ^= bits;
                while (version_bits != 0) {
                        if ((version_bits & 1) == 1)
                                ++errors;
                        version_bits >>= 1;
                }

                if (errors < best_err) {
                        best_version = v;
                        best_err = errors;
                }

                if (errors == 0) /* can we do better than this? */
                        break;
        }

        *version = best_version;

        return best_err;
}

