#ifndef QR_BITSTREAM_H
#define QR_BITSTREAM_H

#include <stddef.h>

/**
 * Note: when writing / reading multiple bits, the
 * _most_ significant bits come first in the stream.
 * (That is, the order you would naturally write the
 * number in binary)
 */

struct bitstream;

struct bitstream * bitstream_create(void);
int                bitstream_resize(struct bitstream *, size_t bits);
void               bitstream_destroy(struct bitstream *);
struct bitstream * bitstream_copy(const struct bitstream *);

void bitstream_seek(struct bitstream *, size_t pos);
size_t bitstream_tell(const struct bitstream *);
size_t bitstream_remaining(const struct bitstream *);
size_t bitstream_size(const struct bitstream *);

unsigned int bitstream_read(struct bitstream *, size_t bits);

void bitstream_unpack(struct bitstream *,
                      unsigned int * result,
                      size_t         count,
                      size_t         bitsize);

int bitstream_write(struct bitstream *,
                    unsigned int value,
                    size_t       bits);

int bitstream_pack(struct bitstream *,
                   const unsigned int * values,
                   size_t               count,
                   size_t               bitsize);

int bitstream_cat(struct bitstream *, const struct bitstream * src);

#endif

