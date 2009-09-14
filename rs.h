#ifndef RS_H
#define RS_H

#include "bitstream.h"

struct bitstream * rs_generate_words(int k, const int * coeff, struct bitstream * data);

#endif

