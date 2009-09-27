#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <qr/code.h>
#include <qr/data.h>

#include "bitstream.h"
#include "code-common.h"

int main() {

        struct qr_code * code;
        struct qr_data * data;
        enum qr_data_type type;
        char *str;
        size_t len;

        type = QR_DATA_NUMERIC;
        str = "01234567";
        len = strlen(str);

        data = qr_create_data(1, type, str, len);
        assert(data);
        assert(qr_get_data_type(data) == type);
        assert(qr_get_data_length(data) == len);

        type = qr_parse_data(data, &str, &len);
        printf("[%d] %d\n", type, (int)len);
        printf("\"%s\"\n", str);

        code = qr_code_create(QR_EC_LEVEL_M, data);
        assert(code);

        printf("Code width %d\n", qr_code_width(code));

        {
                /* Hack: render the code using ANSI terminal art */
                char buf[80*25];
                int x, y;

                qr_bitmap_render(code->modules, buf, 8, 80, 0, 1, 0);
                for (y=0;y<21;++y) {
                        printf("\t|");
                        for (x=0;x<21;++x) {
                                printf("%s  ", buf[y*80+x]?"\033[7m":"\033[0m");
                        }
                        printf("\033[0m|\n");
                }
        }

	return 0;
}

