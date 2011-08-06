#ifndef QR_BITMAP_H
#define QR_BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

struct qr_bitmap {
        unsigned char * bits;
        unsigned char * mask;
        size_t stride;
        size_t width, height;
};

struct qr_bitmap * qr_bitmap_create(size_t width, size_t height, int masked);
void qr_bitmap_destroy(struct qr_bitmap *);

int qr_bitmap_add_mask(struct qr_bitmap *);

struct qr_bitmap * qr_bitmap_clone(const struct qr_bitmap *);

void qr_bitmap_merge(struct qr_bitmap * dest, const struct qr_bitmap * src);

void qr_bitmap_render(const struct qr_bitmap * bmp,
                      void *                   buffer,
                      int                      mod_bits,
                      long                     line_stride,
                      int                      line_repeat,
                      unsigned long            mark,
                      unsigned long            space);

#ifdef __cplusplus
}
#endif

#endif

