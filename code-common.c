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

size_t code_total_capacity(int format)
{
	int side = format * 4 + 17;

	int alignment_side = format > 1 ? (format / 7) + 2 : 0;

	int alignment_count = alignment_side >= 2 ?
		alignment_side * alignment_side - 3 : 0;

	int locator_bits = 8*8*3;

	int format_bits = 8*4 - 1 + (format >= 7 ? 6*3*2 : 0);

	int timing_bits = 2 * (side - 8*2 -
		(alignment_side > 2 ? (alignment_side - 2) * 5 : 0));

	int function_bits = timing_bits + format_bits + locator_bits
		+ alignment_count * 5*5;

	return side * side - function_bits;
}

