#ifndef QR_BITMAP_H
#define QR_BITMAP_H

struct qr_bitmap {
        unsigned char * bits;
        unsigned char * mask;
        size_t stride;
        size_t width, height;
};

struct qr_bitmap * qr_bitmap_create(int width, int height, int masked);
void qr_bitmap_destroy(struct qr_bitmap *);

struct qr_bitmap * qr_bitmap_clone(const struct qr_bitmap *);

void qr_bitmap_merge(struct qr_bitmap * dest, const struct qr_bitmap * src);

void qr_bitmap_render(const struct qr_bitmap * bmp,
                      void *                   buffer,
                      size_t                   mod_bits,
                      size_t                   line_stride,
                      size_t                   line_repeat,
                      unsigned long            mark,
                      unsigned long            space);

int qr_bitmap_write_pbm(const char *             path,
                        const char *             comment,
                        const struct qr_bitmap * bmp);

#endif

