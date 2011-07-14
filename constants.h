#ifndef QR_CONSTANTS_H
#define QR_CONSTANTS_H

#include <qr/types.h>

extern const int QR_ALIGNMENT_LOCATION[40][7];
extern const int QR_DATA_WORD_COUNT[40][4];
/* See spec table 18 (also table 19 for application) */
extern const int QR_RS_BLOCK_COUNT[40][4][2];
extern const enum qr_data_type QR_TYPE_CODES[16];

#endif

