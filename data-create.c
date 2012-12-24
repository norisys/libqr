/**
 * Not "pure" C - only works with ASCII
 */

#include <ctype.h>
#include <stdlib.h>

#include <qr/bitstream.h>
#include <qr/data.h>
#include "constants.h"

static void write_type_and_length(struct qr_data *  data,
                                  enum qr_data_type type,
                                  size_t            length)
{
        (void)qr_bitstream_write(data->bits, QR_TYPE_CODES[type], 4);
        (void)qr_bitstream_write(data->bits, length,
                qr_data_size_field_length(data->version, type));
}

static struct qr_data * encode_numeric(struct qr_data * data,
                                       const unsigned char * input,
                                       size_t           length)
{
        struct qr_bitstream * stream = data->bits;
        size_t bits;

        bits = 4 + qr_data_size_field_length(data->version, QR_DATA_NUMERIC)
                 + qr_data_dpart_length(QR_DATA_NUMERIC, length);

        stream = data->bits;
        if (qr_bitstream_resize(stream,
                        qr_bitstream_size(stream) + bits) != 0)
                return 0;

        write_type_and_length(data, QR_DATA_NUMERIC, length);

        for (; length >= 3; length -= 3) {
                unsigned int x;

                if (!isdigit(input[0]) ||
                    !isdigit(input[1]) ||
                    !isdigit(input[2]))
                        return 0;

                x = (input[0] - '0') * 100
                  + (input[1] - '0') * 10
                  + (input[2] - '0');
                qr_bitstream_write(stream, x, 10);
                input += 3;
        }

        if (length > 0) {
                unsigned int x;

                if (!isdigit(*input))
                        return 0;

                x = *input++ - '0';

                if (length == 2) {
                        if (!isdigit(*input))
                                return 0;
                        x = x * 10 + (*input - '0');
                }

                qr_bitstream_write(stream, x, length == 2 ? 7 : 4);
        }

        return data;
}

static int get_alpha_code(char c)
{
        if (isdigit(c))
                return c - '0';
        else if (isalpha(c))
                return toupper(c) - 'A' + 10;

        switch (c) {
        case ' ': return 36;
        case '$': return 37;
        case '%': return 38;
        case '*': return 39;
        case '+': return 40;
        case '-': return 41;
        case '.': return 42;
        case '/': return 43;
        case ':': return 44;
        default:  return -1;
        }
}

static struct qr_data * encode_alpha(struct qr_data * data,
                                     const unsigned char * input,
                                     size_t           length)
{
        struct qr_bitstream * stream = data->bits;
        size_t bits;

        bits = 4 + qr_data_size_field_length(data->version, QR_DATA_ALPHA)
                 + qr_data_dpart_length(QR_DATA_ALPHA, length);

        stream = data->bits;
        if (qr_bitstream_resize(stream,
                        qr_bitstream_size(stream) + bits) != 0)
                return 0;

        write_type_and_length(data, QR_DATA_ALPHA, length);

        for (; length >= 2; length -= 2) {
                unsigned int x;
                int c1, c2;

                c1 = get_alpha_code(*input++);
                c2 = get_alpha_code(*input++);

                if (c1 < 0 || c2 < 0)
                        return 0;

                x = c1 * 45 + c2;
                qr_bitstream_write(stream, x, 11);
        }

        if (length > 0) {
                int c = get_alpha_code(*input);

                if (c < 0)
                        return 0;

                qr_bitstream_write(stream, c, 6);
        }

        return data;
}

static struct qr_data * encode_8bit(struct qr_data * data,
                                    const unsigned char * input,
                                    size_t           length)
{
        struct qr_bitstream * stream = data->bits;
        size_t bits;

        bits = 4 + qr_data_size_field_length(data->version, QR_DATA_8BIT)
                 + qr_data_dpart_length(QR_DATA_8BIT, length);

        stream = data->bits;
        if (qr_bitstream_resize(stream,
                        qr_bitstream_size(stream) + bits) != 0)
                return 0;

        write_type_and_length(data, QR_DATA_8BIT, length);

        while (length--)
                qr_bitstream_write(stream, *input++, 8);

        return data;
}

static struct qr_data * encode_kanji(struct qr_data * data,
                                     const unsigned char * input,
                                     size_t           length)
{
        return 0;
}

static int calc_min_version(enum qr_data_type type,
                            enum qr_ec_level ec,
                            size_t length)
{
        size_t dbits;
        int version;

        dbits = qr_data_dpart_length(type, length);

        for (version = 1; version <= 40; ++version) {
                if (4 + dbits + qr_data_size_field_length(version, type)
                    < 8 * (size_t) QR_DATA_WORD_COUNT[version - 1][ec ^ 0x1])
                        return version;
        }

        return -1;
}

struct qr_data * qr_data_create(int               version,
                                enum qr_ec_level  ec,
                                enum qr_data_type type,
                                const char *      input,
                                size_t            length)
{
        struct qr_data * data;
        int minver;

        minver = calc_min_version(type, ec, length);

        if (version == 0)
                version = minver;

        if (minver < 0 || version < minver)
                return 0;

        data = malloc(sizeof(*data));
        if (!data)
                return 0;

        data->version = version;
        data->ec      = ec;
        data->bits   = qr_bitstream_create();
        data->offset = 0;

        if (data->bits) {
                struct qr_data * ret;

                switch (type) {
                case QR_DATA_NUMERIC:
                        ret = encode_numeric(data, (const unsigned char *) input, length);
                        break;
                case QR_DATA_ALPHA:
                        ret = encode_alpha(data, (const unsigned char *) input, length);
                        break;
                case QR_DATA_8BIT:
                        ret = encode_8bit(data, (const unsigned char *) input, length);
                        break;
                case QR_DATA_KANJI:
                        ret = encode_kanji(data, (const unsigned char *) input, length);
                        break;
                default:
                        /* unsupported / invalid */
                        ret = 0;
                        break;
                }

                if (!ret) {
                        qr_bitstream_destroy(data->bits);
                        free(data);
                }

                return ret;
        } else {
                free(data);
                return 0;
        }
}

size_t qr_data_dpart_length(enum qr_data_type type, size_t length)
{
        size_t bits;

        switch (type) {
        case QR_DATA_NUMERIC:
                bits = 10 * (length / 3);
                if (length % 3 == 1)
                        bits += 4;
                else if (length % 3 == 2)
                        bits += 7;
                break;
        case QR_DATA_ALPHA:
                bits = 11 * (length / 2)
                        + 6 * (length % 2);
                break;
        case QR_DATA_8BIT:
                bits = 8 * length;
                break;
        case QR_DATA_KANJI:
                /* unsupported */
        default:
                /* unsupported; will be ignored */
                bits = 0;
        }

        return bits;
}

