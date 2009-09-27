#ifndef CODE_COMMON_H
#define CODE_COMMON_H

#include <qr/code.h>
#include "qr-bitstream.h"
#include "qr-bitmap.h"

struct qr_code {
        int                format;
        struct qr_bitmap * modules;
};

int code_side_length(int format);

size_t code_total_capacity(int format);

#endif

