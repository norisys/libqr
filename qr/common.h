#ifndef QR_COMMON_H
#define QR_COMMON_H

#include <qr/types.h>

struct qr_bitmap * qr_mask_apply(const struct qr_bitmap * orig,
                                 unsigned int mask);

size_t qr_code_total_capacity(int version);

int qr_code_width(const struct qr_code *);

#endif

