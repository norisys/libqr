#ifndef QR_COMMON_H
#define QR_COMMON_H

#include <qr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void qr_mask_apply(struct qr_bitmap * bmp, int mask);

size_t qr_code_total_capacity(int version);

int qr_code_width(const struct qr_code *);

/* See table 19 of the spec for the layout of EC data. There are at
 * most two different block lengths, so the total number of data+ec
 * blocks is the sum of block_count[]. The total number of 8-bit
 * words in each kind of block is data_length + ec_length.
 */
void qr_get_rs_block_sizes(int version,
                           enum qr_ec_level ec,
                           int block_count[2],
                           int data_length[2],
                           int ec_length[2]);

#ifdef __cplusplus
}
#endif

#endif

