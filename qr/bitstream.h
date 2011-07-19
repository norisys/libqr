#ifndef QR_BITSTREAM_H
#define QR_BITSTREAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Note: when writing / reading multiple bits, the
 * _most_ significant bits come first in the stream.
 * (That is, the order you would naturally write the
 * number in binary)
 */

struct qr_bitstream;

struct qr_bitstream * qr_bitstream_create(void);
int qr_bitstream_resize(struct qr_bitstream *, size_t bits);
void qr_bitstream_destroy(struct qr_bitstream *);
struct qr_bitstream * qr_bitstream_dup(const struct qr_bitstream *);

void qr_bitstream_seek(struct qr_bitstream *, size_t pos);
size_t qr_bitstream_tell(const struct qr_bitstream *);
size_t qr_bitstream_remaining(const struct qr_bitstream *);
size_t qr_bitstream_size(const struct qr_bitstream *);

unsigned long qr_bitstream_read(struct qr_bitstream *, int bits);

void qr_bitstream_unpack(struct qr_bitstream *,
                         unsigned int * result,
                         size_t         count,
                         int            bitsize);

int qr_bitstream_write(struct qr_bitstream *,
                       unsigned long value,
                       int bits);

int qr_bitstream_pack(struct qr_bitstream *,
                      const unsigned int * values,
                      size_t               count,
                      int                  bitsize);

int qr_bitstream_cat(struct qr_bitstream *,
                     const struct qr_bitstream * src);

int qr_bitstream_copy(struct qr_bitstream * dest,
                      struct qr_bitstream * src,
                      size_t count);

#ifdef __cplusplus
}
#endif

#endif

