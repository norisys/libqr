#include <stdlib.h>
#include <qr/code.h>

#include "code-common.h"

void qr_code_destroy(struct qr_code * code)
{
        free(code->modules);
        free(code);
}

int qr_code_width(const struct qr_code * code)
{
        return code_side_length(code->format);
}

int code_side_length(int format)
{
        return format * 4 + 17;
}

int code_finder_count(int format)
{
        int x = format / 7;
        return format > 1 ? x*x + 4*x + 1 : 0;        
}

size_t code_total_capacity(int format)
{
        /* XXX: figure out the "correct" formula */
        return 160
             +  25 * code_finder_count(format)
             -  10 * (format / 7)
             +   2 * code_side_length(format);
}

