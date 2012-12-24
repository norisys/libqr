#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <qr/code.h>
#include <qr/data.h>
#include <qr/parse.h>

#include "testqr.xbm"

int main(int argc, char ** argv)
{
        struct qr_data * data;
        int ret;

        ret = qr_code_parse(testqr_bits,
                            testqr_width,
                            (testqr_width + CHAR_BIT - 1) / CHAR_BIT,
                            testqr_height,
                            &data);

        fprintf(stderr, "qr_code_parse returned %d\n", ret);

        if (ret == 0) {
                const char * ec_str, * type_str;
                char * data_str;
                size_t data_len;
                enum qr_data_type data_type;

                fprintf(stderr, "version = %d\n", data->version);
                switch (data->ec) {
                case QR_EC_LEVEL_L: ec_str = "L"; break;
                case QR_EC_LEVEL_M: ec_str = "M"; break;
                case QR_EC_LEVEL_Q: ec_str = "Q"; break;
                case QR_EC_LEVEL_H: ec_str = "H"; break;
                default: ec_str = "(unknown)"; break;
                }
                fprintf(stderr, "EC level %s\n", ec_str);

                data_type = qr_parse_data(data, &data_str, &data_len);
                switch (data_type) {
                case QR_DATA_INVALID: type_str = "(invalid)"; break;
                case QR_DATA_ECI: type_str = "ECI"; break;
                case QR_DATA_NUMERIC: type_str = "numeric"; break;
                case QR_DATA_ALPHA: type_str = "alpha"; break;
                case QR_DATA_8BIT: type_str = "8-bit"; break;
                case QR_DATA_KANJI: type_str = "kanji"; break;
                case QR_DATA_MIXED: type_str = "mixed"; break;
                case QR_DATA_FNC1: type_str = "FNC1"; break;
                default: type_str = "(unknown)"; break;
                }
                fprintf(stderr, "Data type: %s; %lu bytes\nContent: %s\n", type_str, (unsigned long) data_len, data_str);
                free(data_str);
                qr_data_destroy(data);
        }

        return 0;
}

