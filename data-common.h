#ifndef DATA_COMMON_H
#define DATA_COMMON_H

#include <qr/data.h>

#include "bitstream.h"

struct qr_data {
        int                format; /* 1 ~ 40 */
        struct bitstream * bits;
        size_t             offset;
};

extern const enum qr_data_type QR_TYPE_CODES[16];

size_t get_size_field_length(int format, enum qr_data_type);

#endif

