#ifndef CODE_COMMON_H
#define CODE_COMMON_H

#include <qr/code.h>
#include "qr-bitstream.h"
#include "qr-bitmap.h"

struct qr_code {
        int                version;
        struct qr_bitmap * modules;
};

size_t code_total_capacity(int version);

#endif

