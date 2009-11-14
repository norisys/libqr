#ifndef QR_CODE_H
#define QR_CODE_H

#include <stddef.h>
#include "types.h"

struct qr_code * qr_code_create(const struct qr_data * data);

void qr_code_destroy(struct qr_code *);

int qr_code_width(const struct qr_code *);

struct qr_code * qr_code_parse(const void * buffer,
                               size_t       line_bits,
                               size_t       line_stride,
                               size_t       line_count);

#endif

