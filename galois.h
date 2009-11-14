#ifndef QR_GALOIS_H
#define QR_GALOIS_H

unsigned long gf_residue(unsigned long a, unsigned long m);

struct qr_bitstream * rs_generate_words(struct qr_bitstream * data,
                                        size_t data_words,
                                        size_t rs_words);

#endif

