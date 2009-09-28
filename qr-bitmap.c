#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "qr-bitmap.h"

struct qr_bitmap * qr_bitmap_create(int width, int height, int masked)
{
        struct qr_bitmap * out;
        size_t size;

        out = malloc(sizeof(*out));
        if (!out)
                goto fail;

        out->width = width;
        out->height = height;
        out->stride = (width / CHAR_BIT) + (width % CHAR_BIT ? 1 : 0);

        size = out->stride * height;

        out->mask = 0;
        out->bits = malloc(size);
        if (!out->bits)
                goto fail;
        memset(out->bits, 0, size);

        if (masked) {
                out->mask = malloc(out->stride * width);
                if (!out->mask)
                        goto fail;
                memset(out->mask, 0xFF, size);
        }

        return out;

fail:
        qr_bitmap_destroy(out);
        return 0;
}

void qr_bitmap_destroy(struct qr_bitmap * bmp)
{
        if (bmp) {
                free(bmp->bits);
                free(bmp->mask);
                free(bmp);
        }
}

struct qr_bitmap * qr_bitmap_clone(const struct qr_bitmap * src)
{
        struct qr_bitmap * bmp;
        size_t size;

        bmp = qr_bitmap_create(src->width, src->height, !!src->mask);
        if (!bmp)
                return 0;

        assert(bmp->stride == src->stride);

        size = bmp->width * bmp->stride;
        memcpy(bmp->bits, src->bits, size);
        if (bmp->mask)
                memcpy(bmp->mask, src->mask, size);

        return bmp;
}

void qr_bitmap_merge(struct qr_bitmap * dest, const struct qr_bitmap * src)
{
        unsigned char * d, * s, * m;
        size_t n;

        assert(dest->stride == src->stride);
        assert(dest->height == src->height);
        assert(src->mask);

        n = dest->stride * dest->height;
        d = dest->bits;
        s = src->bits;
        m = src->mask;

        while (n--) {
                *d   &= ~*m;
                *d++ |= *s++ & *m++;
        }
}

