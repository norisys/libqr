#ifndef CODE_COMMON_H
#define CODE_COMMON_H

#include <qr/code.h>
#include "bitstream.h"

struct qr_code {
        int              format;
        unsigned char *  modules;
        size_t           line_stride;
};

int code_side_length(int format);

size_t code_total_capacity(int format);

#endif

