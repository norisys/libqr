#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <qr/bitstream.h>
#include <qr/data.h>
#include "constants.h"

static enum qr_data_type read_data_type(struct qr_bitstream * stream)
{
        const size_t length = 4;
        unsigned int type;

        if (qr_bitstream_remaining(stream) < length)
                return QR_DATA_INVALID;

        type = qr_bitstream_read(stream, length);
        assert(type < 16);

        return QR_TYPE_CODES[type];
}

static enum qr_data_type parse_numeric(const struct qr_data * data,
                                       char **                output,
                                       size_t *               length)
{
        struct qr_bitstream * stream;
        size_t field_len, bits;
	unsigned int digits;
	unsigned int chunk;
	char * p, * buffer;

        stream = data->bits;
        buffer = 0;

        field_len = qr_data_size_field_length(data->version, QR_DATA_NUMERIC);
        if (qr_bitstream_remaining(stream) < field_len)
                goto invalid;

	digits = qr_bitstream_read(stream, field_len);

	bits = (digits / 3) * 10;
	if (digits % 3 == 1)
		bits += 4;
	else if (digits % 3 == 2)
		bits += 7;

	if (qr_bitstream_remaining(stream) < bits)
		goto invalid;

	buffer = malloc(digits + 1);
	if (!buffer)
		goto invalid;

	p = buffer;

	for (; bits >= 10; bits -= 10) {
		chunk = qr_bitstream_read(stream, 10);
                if (chunk >= 1000)
                        goto invalid;
		sprintf(p, "%03u", chunk);
                p += 3;
	}

        if (bits > 0) {
                chunk = qr_bitstream_read(stream, bits);
                if (chunk >= (bits >= 7 ? 100 : 10))
                        goto invalid;
                sprintf(p, "%0*u", bits >= 7 ? 2 : 1, chunk);
        }

        *output = buffer;
        *length = digits;

        return QR_DATA_NUMERIC;
invalid:
        free(buffer);
        return QR_DATA_INVALID;
}

static enum qr_data_type parse_alpha(const struct qr_data * data,
                                     char **                output,
                                     size_t *               length)
{
        static const char charset[45] =
                "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
        struct qr_bitstream * stream;
        size_t field_len, bits;
	unsigned int chars;
	unsigned int chunk;
	char * p, * buffer;

        stream = data->bits;
        buffer = 0;

        field_len = qr_data_size_field_length(data->version, QR_DATA_ALPHA);
        if (qr_bitstream_remaining(stream) < field_len)
                goto invalid;

	chars = qr_bitstream_read(stream, field_len);

	bits = (chars / 2) * 11;
	if (chars % 2 == 1)
		bits += 6;

	if (qr_bitstream_remaining(stream) < bits)
		goto invalid;

	buffer = malloc(chars + 1);
	if (!buffer)
		goto invalid;

	p = buffer;

	for (; bits >= 11; bits -= 11) {
                unsigned int c1, c2;
		chunk = qr_bitstream_read(stream, 11);
                c1 = chunk / 45;
                c2 = chunk % 45;
                if (c1 >= 45)
                        goto invalid;
                *p++ = charset[c1];
                *p++ = charset[c2];
	}

        if (bits > 0) {
                chunk = qr_bitstream_read(stream, bits);
                if (chunk >= 45)
                        goto invalid;
                *p = charset[chunk];
        }

        *output = buffer;
        *length = chars;

        return QR_DATA_ALPHA;
invalid:
        free(buffer);
        return QR_DATA_INVALID;
}

static enum qr_data_type parse_8bit(const struct qr_data * data,
                                    char **                output,
                                    size_t *               length)
{
        struct qr_bitstream * stream;
        size_t field_len;
	unsigned int bytes;
	char * p;

        stream = data->bits;

        field_len = qr_data_size_field_length(data->version, QR_DATA_8BIT);
        if (qr_bitstream_remaining(stream) < field_len)
                return QR_DATA_INVALID;

	bytes = qr_bitstream_read(stream, field_len);

	if (qr_bitstream_remaining(stream) < bytes * 8)
		return QR_DATA_INVALID;

	*output = malloc(bytes + 1);
	if (!*output)
		return QR_DATA_INVALID;

        p = *output;
        *length = bytes;

        while (bytes-- > 0)
                *p++ = qr_bitstream_read(stream, 8);

        *p = '\0';

        return QR_DATA_8BIT;
}

static enum qr_data_type parse_kanji(const struct qr_data * data,
                                     char **                output,
                                     size_t *               length)
{
        return QR_DATA_INVALID;
}

enum qr_data_type qr_data_type(const struct qr_data * data)
{
        qr_bitstream_seek(data->bits, data->offset);
        return read_data_type(data->bits);
}

int qr_get_data_length(const struct qr_data * data) 
{
        size_t field_len;
        enum qr_data_type type;

        qr_bitstream_seek(data->bits, data->offset);

        type = read_data_type(data->bits);

        switch (type) {
        case QR_DATA_NUMERIC:
        case QR_DATA_ALPHA:
        case QR_DATA_8BIT:
        case QR_DATA_KANJI:
                field_len = qr_data_size_field_length(data->version, type);
                break;
        default:
                /* unsupported / invalid */
                return -1;
        }

        if (qr_bitstream_remaining(data->bits) < field_len)
                return -1;

        return (int) qr_bitstream_read(data->bits, field_len);
}

enum qr_data_type qr_parse_data(const struct qr_data * input,
                                char **                output,
                                size_t *               length)
{
        qr_bitstream_seek(input->bits, input->offset);

        *output = NULL;
        *length = 0;

        switch (read_data_type(input->bits)) {
        case QR_DATA_NUMERIC:
                return parse_numeric(input, output, length);
        case QR_DATA_ALPHA:
                return parse_alpha(input, output, length);
        case QR_DATA_8BIT:
                return parse_8bit(input, output, length);
        case QR_DATA_KANJI:
                return parse_kanji(input, output, length);
        default:
                /* unsupported / invalid */
                return QR_DATA_INVALID;
        }
}

