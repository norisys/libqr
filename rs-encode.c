#include <assert.h>
#include <stdlib.h>
#include "bitstream.h"
#include "rs.h"

struct bitstream * rs_generate_words(int k, const int * coeff, struct bitstream * data)
{
        struct bitstream * ec = 0;
        unsigned int * b = 0;
        size_t n, i, dlen;

        dlen = bitstream_size(data);
        assert(dlen % 8 == 0);
        dlen /= 8;

        ec = bitstream_create();
        if (!ec)
                return 0;

        if (bitstream_resize(ec, k * 8) != 0)
                goto fail;

        b = calloc(k, sizeof(*b));
        if (!b)
                goto fail;

        /* First, prepare the registers (b) with data bits. Note that
         * the registers are in reverse of the normal order
         */
        bitstream_seek(data, 0);
        for (n = 0; n < dlen; ++n) {
                unsigned int x = b[0] + bitstream_read(data, 8);
                for (i = 0; i < k-1; ++i)
                        b[i] = b[i+1] + coeff[(k-1) - i] * x;
                b[k-1] = coeff[0] * x;
        }

        /* Read off the registers */
        bitstream_pack(ec, b, k, 8);

        free(b);
        return ec;
fail:
        bitstream_destroy(ec);
        return 0;
}

