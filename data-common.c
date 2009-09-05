#include <stdlib.h>
#include <qr/data.h>

#include "bitstream.h"
#include "data-common.h"

const enum qr_data_type QR_TYPE_CODES[16] = {
        QR_DATA_INVALID,        /* 0000 */
        QR_DATA_NUMERIC,        /* 0001 */
        QR_DATA_ALPHA,          /* 0010 */
        QR_DATA_MIXED,          /* 0011 */
        QR_DATA_8BIT,           /* 0100 */
        QR_DATA_FNC1,           /* 0101 */
        QR_DATA_INVALID,        /* 0110 */
        QR_DATA_ECI,            /* 0111 */
        QR_DATA_KANJI,          /* 1000 */
        QR_DATA_FNC1,           /* 1001 */
        QR_DATA_INVALID,        /* 1010 */
        QR_DATA_INVALID,        /* 1011 */
        QR_DATA_INVALID,        /* 1100 */
        QR_DATA_INVALID,        /* 1101 */
        QR_DATA_INVALID,        /* 1110 */
        QR_DATA_INVALID,        /* 1111 */
};

void qr_free_data(struct qr_data * data)
{
        bitstream_destroy(data->bits);
        free(data);
}

size_t get_size_field_length(int format, enum qr_data_type type)
{
        static const size_t QR_SIZE_LENGTHS[3][4] = {
                { 10,  9,  8,  8 },
                { 12, 11, 16, 10 },
                { 14, 13, 16, 12 }
        };
        int row, col;

        switch (type) {
        case QR_DATA_NUMERIC:   col = 0; break;
        case QR_DATA_ALPHA:     col = 1; break;
        case QR_DATA_8BIT:      col = 2; break;
        case QR_DATA_KANJI:     col = 3; break;
        default:                return 0;
        }

        if (format < 10)
                row = 0;
        else if (format < 27)
                row = 1;
        else
                row = 2;

        return QR_SIZE_LENGTHS[row][col];
}

