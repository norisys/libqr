#ifndef QR_CODE_H
#define QR_CODE_H

#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct qr_code {
        int                version;
        struct qr_bitmap * modules;
};

struct qr_code * qr_code_create(const struct qr_data * data);

void qr_code_destroy(struct qr_code *);

#ifdef __cplusplus
}
#endif

#endif

