#ifndef QR_MASK_H
#define QR_MASK_H

#include "qr-bitmap.h"

struct qr_bitmap * qr_mask_apply(const struct qr_bitmap * orig,
                                 unsigned int mask);

#endif

