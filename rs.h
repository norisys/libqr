#ifndef RS_H
#define RS_H

#include "bitstream.h"

struct bitstream * rs_generate_words(struct bitstream * data,
                                     size_t data_words,
                                     size_t rs_words);

#endif

