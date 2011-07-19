#ifndef QR_CONSTANTS_H
#define QR_CONSTANTS_H

#include <qr/types.h>

/* XOR mask for format data: 101 0100 0001 0010 */
static const unsigned int QR_FORMAT_MASK = 0x5412;

/* Format info EC polynomial
 * G(x) = x^10 + x^8 + x^5 + x^4 + x^2 + x + 1
 */
static const unsigned int QR_FORMAT_POLY = 0x537;

/* Version info EC polynomial
 * G(x) = x^12 + x^11 + x^10 + x^9 + x^8 + x^5 + x^2 + 1
 */
static const unsigned int QR_VERSION_POLY = 0x1F25;

/* A QR-code word is always 8 bits, but CHAR_BIT might not be */
static const int QR_WORD_BITS = 8;

extern const int QR_ALIGNMENT_LOCATION[40][7];
extern const int QR_DATA_WORD_COUNT[40][4];
/* See qr_get_rs_block_sizes() */
extern const int QR_RS_BLOCK_COUNT[40][4][2];
extern const enum qr_data_type QR_TYPE_CODES[16];

#endif

