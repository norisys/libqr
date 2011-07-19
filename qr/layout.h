#ifndef QR_CODE_LAYOUT_H
#define QR_CODE_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

struct qr_iterator;

void qr_layout_init_mask(struct qr_code *);

struct qr_iterator * qr_layout_begin(struct qr_code * code);
unsigned int qr_layout_read(struct qr_iterator *);
void qr_layout_write(struct qr_iterator *, unsigned int);
void qr_layout_end(struct qr_iterator *);

#ifdef __cplusplus
}
#endif

#endif

