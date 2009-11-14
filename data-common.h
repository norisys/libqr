#ifndef DATA_COMMON_H
#define DATA_COMMON_H

#include <qr/data.h>

#include "qr-bitstream.h"

struct qr_data {
        int                version; /* 1 ~ 40 */
        enum qr_ec_level      ec;
        struct qr_bitstream * bits;
        size_t             offset;
};

extern const enum qr_data_type QR_TYPE_CODES[16];

size_t get_size_field_length(int version, enum qr_data_type);

size_t qr_data_dpart_length(enum qr_data_type type, size_t nchars);

#endif

