#ifndef QR_DATA_H
#define QR_DATA_H

#include <stddef.h>
#include "types.h"

enum qr_data_type {
        QR_DATA_INVALID = -1,
        QR_DATA_ECI     =  7,
        QR_DATA_NUMERIC =  1,
        QR_DATA_ALPHA   =  2,
        QR_DATA_8BIT    =  4,
        QR_DATA_KANJI   =  8, /* JIS X 0208 */
        QR_DATA_MIXED   =  3,
        QR_DATA_FNC1    =  9
};

struct qr_data * qr_create_data(int               format, /* 1 ~ 40 */
                                enum qr_data_type type,
                                const char *      input,
                                size_t            length);

void qr_free_data(struct qr_data *);

enum qr_data_type qr_get_data_type(const struct qr_data *);

int qr_get_data_length(const struct qr_data *);

enum qr_data_type qr_parse_data(const struct qr_data * input,
                                char **                output,
                                size_t *               length);

#endif

