#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#include <qr/bitmap.h>
#include <qr/bitstream.h>
#include <qr/code.h>
#include <qr/common.h>
#include <qr/data.h>
#include <qr/layout.h>
#include "constants.h"
#include "galois.h"

#define MIN(a, b) ((b) < (a) ? (b) : (a))

static int mask_data(struct qr_code * code);
static int score_mask(const struct qr_bitmap * bmp);
static int score_runs(const struct qr_bitmap * bmp, int base);
static int count_2blocks(const struct qr_bitmap * bmp);
static int count_locators(const struct qr_bitmap * bmp);
static int calc_bw_balance(const struct qr_bitmap * bmp);
static int get_px(const struct qr_bitmap * bmp, int x, int y);
static int get_mask(const struct qr_bitmap * bmp, int x, int y);
static int draw_format(struct qr_bitmap * bmp,
                        struct qr_code * code,
                        enum qr_ec_level ec,
                        int mask);
static unsigned int calc_format_bits(enum qr_ec_level ec, int mask);
static unsigned long calc_version_bits(int version);

static void setpx(struct qr_bitmap * bmp, int x, int y)
{
        size_t off = y * bmp->stride + x / CHAR_BIT;
        unsigned char bit = 1 << (x % CHAR_BIT);

        bmp->bits[off] |= bit;
}

static void draw_locator(struct qr_bitmap * bmp, int x, int y)
{
        int i;
        for (i = 0; i < 6; ++i) {
                setpx(bmp, x + i, y + 0);
                setpx(bmp, x + 6, y + i);
                setpx(bmp, x + i + 1, y + 6);
                setpx(bmp, x, y + i + 1);
        }
        for (i = 0; i < 9; ++i)
                setpx(bmp, x + 2 + i % 3, y + 2 + i / 3);
}

static int draw_functional(struct qr_code * code,
                           enum qr_ec_level ec,
                           unsigned int mask)
{
        struct qr_bitmap * bmp;
        int dim = qr_code_width(code);
        int i;
        int x, y;
        int am_side;

        bmp = qr_bitmap_create(dim, dim, 0);
        if (!bmp)
                return -1;

        /* Locator pattern */
        draw_locator(bmp, 0, 0);
        draw_locator(bmp, 0, dim - 7);
        draw_locator(bmp, dim - 7, 0);

        /* Timing pattern */
        for (i = 8; i < dim - 8; i += 2) {
                setpx(bmp, i, 6);
                setpx(bmp, 6, i);
        }

        /* Alignment pattern */
        am_side = code->version > 1 ? (code->version / 7) + 2 : 0;
        for (y = 0; y < am_side; ++y) {
                const int * am_pos = QR_ALIGNMENT_LOCATION[code->version - 1];

                for (x = 0; x < am_side; ++x) {
                        if ((x == 0 && y == 0) ||
                            (x == 0 && y == am_side - 1) ||
                            (x == am_side - 1 && y == 0))
                                continue;

                        for (i = -2; i < 2; ++i) {
                                setpx(bmp, am_pos[x] + i, am_pos[y] - 2);
                                setpx(bmp, am_pos[x] + 2, am_pos[y] + i);
                                setpx(bmp, am_pos[x] - i, am_pos[y] + 2);
                                setpx(bmp, am_pos[x] - 2, am_pos[y] - i);
                        }
                        setpx(bmp, am_pos[x], am_pos[y]);
                }
        }

        /* Format info */
        setpx(bmp, 8, dim - 8);
        if (draw_format(bmp, code, ec, mask) != 0)
                return -1;

        /* Merge data */
        qr_bitmap_merge(bmp, code->modules);
        qr_bitmap_destroy(code->modules);
        code->modules = bmp;

        return 0;
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

static struct qr_bitstream * make_data(int version,
                                       enum qr_ec_level   ec,
                                       struct qr_bitstream * data)
{
        const size_t total_bits = qr_code_total_capacity(version);
        const size_t total_data = QR_DATA_WORD_COUNT[version - 1][ec ^ 0x1];
        int block_count[2], data_length[2], ec_length[2];
        int total_blocks;
        int i, w;
        struct qr_bitstream * dcopy = 0;
        struct qr_bitstream * out = 0;
        struct qr_bitstream ** blocks = 0;

        /* Set up the output stream */
        out = qr_bitstream_create();
        if (!out)
                return 0;

        if (qr_bitstream_resize(out, total_bits) != 0)
                goto fail;

        qr_get_rs_block_sizes(version, ec, block_count, data_length, ec_length);
        total_blocks = block_count[0] + block_count[1];

        /* Make a copy of the data and pad it */
        dcopy = qr_bitstream_dup(data);
        if (!dcopy)
                goto fail;

        if (pad_data(dcopy, total_data * QR_WORD_BITS) != 0)
                goto fail;

        /* Make space for the RS blocks */
        blocks = malloc(total_blocks * sizeof(*blocks));
        if (!blocks)
                goto fail;
        for (i = 0; i < total_blocks; ++i)
                blocks[i] = NULL;

        /* Generate RS codewords */
        qr_bitstream_seek(dcopy, 0);
        for (i = 0; i < total_blocks; ++i) {
                int type = (i >= block_count[0]);
                blocks[i] = rs_generate_words(dcopy,
                                              data_length[type],
                                              ec_length[type]);
                if (!blocks[i]) {
                        while (i--)
                                qr_bitstream_destroy(blocks[i]);
                        free(blocks);
                        blocks = 0;
                        goto fail;
                }
        }

        /* Finally, write everything out in the correct order */
        assert(block_count[1] == 0 || data_length[1] >= data_length[0]);
        for (w = 0; w < data_length[block_count[1] ? 1 : 0]; ++w) {
                for (i = (w >= data_length[0] ? block_count[0] : 0); i < total_blocks; ++i) {
                        long di = w + i * data_length[0]
                                + (i > block_count[0] ?
                                        (i - block_count[0]) * (data_length[1] - data_length[0])
                                        : 0);

                        qr_bitstream_seek(dcopy, di * QR_WORD_BITS);
                        qr_bitstream_copy(out, dcopy, QR_WORD_BITS);
                }
        }
        for (i = 0; i < total_blocks; ++i)
                qr_bitstream_seek(blocks[i], 0);
        assert(block_count[1] == 0 || ec_length[1] == ec_length[0]);
        for (w = 0; w < ec_length[0]; ++w)
                for (i = 0; i < total_blocks; ++i)
                        qr_bitstream_copy(out, blocks[i], QR_WORD_BITS);

        qr_bitstream_write(out, 0, total_bits % QR_WORD_BITS);

exit:
        if (blocks) {
                while (total_blocks--)
                       qr_bitstream_destroy(blocks[total_blocks]);
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

struct qr_code * qr_code_create(const struct qr_data * data)
{
        struct qr_code * code;
        struct qr_bitstream * bits = 0;
        struct qr_iterator * layout;
        int mask;
        size_t dim;

        code = malloc(sizeof(*code));
        if (!code)
                return 0;

        code->version = data->version;
        dim = qr_code_width(code);
        code->modules = qr_bitmap_create(dim, dim, 1);

        if (!code->modules)
                goto fail;

        bits = make_data(data->version, data->ec, data->bits);
        if (!bits)
                goto fail;

        qr_layout_init_mask(code);

        layout = qr_layout_begin(code);
        if (!layout)
                goto fail;

        qr_bitstream_seek(bits, 0);
        while (qr_bitstream_remaining(bits) >= (size_t) QR_WORD_BITS)
                qr_layout_write(layout, qr_bitstream_read(bits, QR_WORD_BITS));
        qr_layout_end(layout);

        mask = mask_data(code);
        if (mask < 0)
                goto fail;

        if (draw_functional(code, data->ec, mask) != 0)
                goto fail;

exit:
        if (bits)
                qr_bitstream_destroy(bits);

        return code;

fail:
        qr_code_destroy(code);
        code = 0;
        goto exit;
}

static int mask_data(struct qr_code * code)
{
        struct qr_bitmap * mask, * test;
        int selected, score;
        int i, best;

        mask = 0;

        /* Generate bitmap for each mask and evaluate */
        for (i = 0; i < 8; ++i) {
                test = qr_bitmap_clone(code->modules);
                if (!test) {
                        qr_bitmap_destroy(mask);
                        return -1;
                }
                qr_mask_apply(test, i);
                score = score_mask(test);
                if (!mask || score < best) {
                        best = score;
                        selected = i;
                        qr_bitmap_destroy(mask);
                        mask = test;
                } else {
                        qr_bitmap_destroy(test);
                }
        }

        qr_bitmap_destroy(code->modules);
        code->modules = mask;

        return selected;
}

static int score_mask(const struct qr_bitmap * bmp)
{
        const int N[4] = { 3, 3, 40, 10 };
        int score = 0;

        score += score_runs(bmp, N[0]);
        score += N[1] * count_2blocks(bmp);
        score += N[2] * count_locators(bmp);
        score += N[3] * ((abs(calc_bw_balance(bmp) - 50) + 4) / 5);

        return score;
}

static int score_runs(const struct qr_bitmap * bmp, int base)
{
        /* Runs of 5+n bits -> N[0] + i */
        size_t x, y;
        int flip;
        int score = 0;
        int count, last;

        for (flip = 0; flip <= 1; ++flip) {
                for (y = 0; y < bmp->height; ++y) {
                        count = 0;
                        for (x = 0; x < bmp->width; ++x) {
                                int mask, bit;
                                if (flip) {
                                        mask = get_mask(bmp, y, x);
                                        bit = get_mask(bmp, y, x);
                                } else {
                                        mask = get_mask(bmp, x, y);
                                        bit = get_px(bmp, x, y);
                                }

                                if (mask &&
                                    (count == 0 || !!bit == !!last)) {
                                        ++count;
                                        last = bit;
                                } else {
                                        if (count >= 5)
                                                score += base + count - 5;
                                        count = 0;
                                }
                        }
                }
        }

        return score;
}

static int count_2blocks(const struct qr_bitmap * bmp)
{
        /* Count the number of 2x2 blocks (on or off) */
        size_t x, y;
        int count = 0;

        /* Slow and stupid */
        for (y = 0; y < bmp->height - 1; ++y) {
                for (x = 0; x < bmp->width - 1; ++x) {
                        if (get_mask(bmp, x, y) &&
                            get_mask(bmp, x+1, y) &&
                            get_mask(bmp, x, y+1) &&
                            get_mask(bmp, x+1, y+1)) {
                                int v[4];
                                v[0] = get_px(bmp, x, y);
                                v[1] = get_px(bmp, x+1, y);
                                v[2] = get_px(bmp, x, y+1);
                                v[3] = get_px(bmp, x+1, y+1);
                                if (!(v[0] || v[1] || v[2] || v[3]) ||
                                     (v[0] && v[1] && v[2] && v[3])) {
                                        ++count;
                                }
                        }
                }
        }

        return count;
}

static int count_locators(const struct qr_bitmap * bmp)
{
        /* 1:1:3:1:1 patterns -> N[2] */
        size_t x, y;
        int flip;
        int count = 0;

        for (flip = 0; flip <= 1; ++flip) {
                for (y = 0; y < bmp->height - 7; ++y) {
                        for (x = 0; x < bmp->width - 7; ++x) {
                                int bits[7];
                                int i;

                                for (i = 0; i < 7; ++i)
                                        if (!(flip ? get_mask(bmp, y, x+i) : get_mask(bmp, x+i, y)))
                                                continue;

                                for (i = 0; i < 7; ++i)
                                        bits[i] = flip ? get_px(bmp, y, x+i) : get_px(bmp, x+i, y);

                                if (!bits[0])
                                        for (i = 0; i < 7; ++i)
                                                bits[i] = !bits[i];

                                if ( bits[0] && !bits[1] &&  bits[2] &&
                                     bits[3] &&  bits[4] && !bits[5] &&
                                     bits[6])
                                        ++count;
                        }
                }
        }

        return count;
}

static int calc_bw_balance(const struct qr_bitmap * bmp)
{
        /* Calculate the proportion (in percent) of "on" bits */
        size_t x, y;
        unsigned char bit;
        long on, total;

        assert(bmp->mask); /* we only need this case */

        on = total = 0;
        for (y = 0; y < bmp->height; ++y) {
                size_t off = y * bmp->stride;
                unsigned char * bits = bmp->bits + off;
                unsigned char * mask = bmp->mask + off;

                for (x = 0; x < bmp->width / CHAR_BIT; ++x, ++bits, ++mask) {
                        for (bit = 1; bit; bit <<= 1) {
                                int m = *mask & bit;

                                total += !!m;
                                on += !!(*bits & m);
                        }
                }
        }

        return (on * 100) / total;
}

static int get_px(const struct qr_bitmap * bmp, int x, int y)
{
        size_t off = y * bmp->stride + x / CHAR_BIT;
        unsigned char bit = 1 << (x % CHAR_BIT);

        return bmp->bits[off] & bit;
}

static int get_mask(const struct qr_bitmap * bmp, int x, int y)
{
        size_t off = y * bmp->stride + x / CHAR_BIT;
        unsigned char bit = 1 << (x % CHAR_BIT);

        return bmp->mask[off] & bit;
}

static int draw_format(struct qr_bitmap * bmp,
                        struct qr_code * code,
                        enum qr_ec_level ec,
                        int mask)
{
        int i;
        size_t dim;
        long bits;

        dim = bmp->width;

        bits = calc_format_bits(ec, mask);
        if (bits < 0)
                return -1;

        for (i = 0; i < 8; ++i) {
                if (bits & 0x1) {
                        setpx(bmp, 8, i + (i > 5));
                        setpx(bmp, dim - 1 - i, 8);
                }
                bits >>= 1;
        }

        for (i = 0; i < 7; ++i) {
                if (bits & 0x1) {
                        setpx(bmp, 8, dim - 7 + i);
                        setpx(bmp, 6 - i + (i == 0), 8);
                }
                bits >>= 1;
        }

        if (code->version >= 7) {
                bits = calc_version_bits(code->version);

                for (i = 0; i < 18; ++i) {
                        if (bits & 0x1) {
                                int a = i % 3, b = i / 3;
                                setpx(bmp, dim - 11 + a, b);
                                setpx(bmp, b, dim - 11 + a);
                        }
                        bits >>= 1;
                }
        }

        return 0;
}

static unsigned int calc_format_bits(enum qr_ec_level ec, int mask)
{
        unsigned int bits;

        bits = (ec & 0x3) << 3 | (mask & 0x7);

        /* Compute (15, 5) BCH code */

        bits <<= 15 - 5;
        bits |= (unsigned int)gf_residue(bits, QR_FORMAT_POLY);

        bits ^= QR_FORMAT_MASK;

        return bits;
}

static unsigned long calc_version_bits(int version)
{
        unsigned long bits;

        bits = version & 0x3F;

        /* (18, 6) BCH code */

        bits <<= 18 - 6;
        bits |= gf_residue(bits, QR_VERSION_POLY);

        return bits;
}

