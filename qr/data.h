#ifndef QR_DATA_H
#define QR_DATA_H

#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct qr_data {
        int                   version; /* 1 ~ 40 */
        enum qr_ec_level      ec;
        struct qr_bitstream * bits;
        size_t                offset;
};

struct qr_data * qr_data_create(int               format, /* 1 ~ 40; 0=auto */
                                enum qr_ec_level  ec,
                                enum qr_data_type type,
                                const char *      input,
                                size_t            length);

void qr_data_destroy(struct qr_data *);

enum qr_data_type qr_data_type(const struct qr_data *);

size_t qr_data_length(const struct qr_data *);
size_t qr_data_size_field_length(int version, enum qr_data_type);
size_t qr_data_dpart_length(enum qr_data_type type, size_t nchars);

enum qr_data_type qr_parse_data(const struct qr_data * input,
                                char **                output,
                                size_t *               length);

#ifdef __cplusplus
}
#endif

#endif

