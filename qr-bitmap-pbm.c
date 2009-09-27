#include <limits.h>
#include <stdio.h>
#include "qr-bitmap.h"

int qr_bitmap_write_pbm(const char *             path,
                        const char *             comment,
                        const struct qr_bitmap * bmp)
{
        FILE * out;
        size_t count, x, y;

        out = fopen(path, "w");
        if (!out)
                return -1;

        count = 0;

        count += fputs("P1\n", out);

        if (comment)
                count += fprintf(out, "# %s\n", comment);

        count += fprintf(out, "%u %u\n",
                         (unsigned)bmp->width,
                         (unsigned)bmp->height);

        for (y = 0; y < bmp->height; ++y) {
                unsigned char * row = bmp->bits + y * bmp->stride;

                for (x = 0; x < bmp->width; ++x) {
                        int bit = row[x / CHAR_BIT] & (1 << x % CHAR_BIT);

                        if (x > 0)
                                fputc(' ', out);
                        fputc(bit ? '1' : '0', out);
                }
                count += fputc('\n', out);
        }

        if (ferror(out))
                count = -1;

        fclose(out);

        return count;
}

