#ifndef QR_TYPES_H
#define QR_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

struct qr_data;
struct qr_code;

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

enum qr_ec_level {
        QR_EC_LEVEL_L = 0x1,
        QR_EC_LEVEL_M = 0x0,
        QR_EC_LEVEL_Q = 0x3,
        QR_EC_LEVEL_H = 0x2
};

#ifdef __cplusplus
}
#endif

#endif

