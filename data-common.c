#include <stdlib.h>

#include <qr/bitstream.h>
#include <qr/data.h>

void qr_data_destroy(struct qr_data * data)
{
        qr_bitstream_destroy(data->bits);
        free(data);
}

size_t qr_data_size_field_length(int version, enum qr_data_type type)
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

        if (version < 10)
                row = 0;
        else if (version < 27)
                row = 1;
        else
                row = 2;

        return QR_SIZE_LENGTHS[row][col];
}

