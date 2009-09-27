#ifndef RS_H
#define RS_H

#include "qr-bitstream.h"

struct qr_bitstream * rs_generate_words(struct qr_bitstream * data,
                                     size_t data_words,
                                     size_t rs_words);

#endif

