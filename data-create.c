/**
 * Not "pure" C - only works with ASCII
 */

/** XXX: check that the data will fit! **/
/** NOTE: should store ec type in qr_data **/

#include <ctype.h>
#include <stdlib.h>
#include <qr/data.h>

#include "qr-bitstream.h"
#include "data-common.h"

static void write_type_and_length(struct qr_data *  data,
                                  enum qr_data_type type,
                                  size_t            length)
{
        (void)qr_bitstream_write(data->bits, QR_TYPE_CODES[type], 4);
        (void)qr_bitstream_write(data->bits, length,
                get_size_field_length(data->version, type));
}

static struct qr_data * encode_numeric(struct qr_data * data,
                                       const char *     input,
                                       size_t           length)
{
        struct qr_bitstream * stream = data->bits;
        size_t bits;

        bits = 4 + get_size_field_length(data->version, QR_DATA_NUMERIC)
                 + 10 * (length / 3);

        if (length % 3 == 1)
                bits += 4;
        else if (length % 3 == 2)
                bits += 7;

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
                                     const char *     input,
                                     size_t           length)
{
        struct qr_bitstream * stream = data->bits;
        size_t bits;

        bits = 4 + get_size_field_length(data->version, QR_DATA_ALPHA)
                 + 11 * (length / 2)
                 + 6 * (length % 2);

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
                                    const char *     input,
                                    size_t           length)
{
        struct qr_bitstream * stream = data->bits;
        size_t bits;

        bits = 4 + get_size_field_length(data->version, QR_DATA_8BIT)
                 + 8 * length;

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
                                     const char *     input,
                                     size_t           length)
{
        return 0;
}

struct qr_data * qr_create_data(int               version,
                                enum qr_data_type type,
                                const char *      input,
                                size_t            length)
{
        struct qr_data * data;

        if (version < 1 || version > 40)
                return 0;

        data = malloc(sizeof(*data));
        if (!data)
                return 0;

        data->version = version;
        data->bits   = qr_bitstream_create();
        data->offset = 0;

        if (data->bits) {
                struct qr_data * ret;

                switch (type) {
                case QR_DATA_NUMERIC:
                        ret = encode_numeric(data, input, length); break;
                case QR_DATA_ALPHA:
                        ret = encode_alpha(data, input, length); break;
                case QR_DATA_8BIT:
                        ret = encode_8bit(data, input, length); break;
                case QR_DATA_KANJI:
                        ret = encode_kanji(data, input, length); break;
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

