#ifndef QR_PARSE_H
#define QR_PARSE_H

#include "data.h"

int qr_code_parse(const void *      buffer,
                  size_t            line_bits,
                  size_t            line_stride,
                  size_t            line_count,
                  struct qr_data ** data);

int qr_decode_format(unsigned bits, enum qr_ec_level * ec, int * mask);
int qr_decode_version(unsigned long bits, int * version);

#endif

