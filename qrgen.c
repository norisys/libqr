#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <png.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qr/bitmap.h>
#include <qr/bitstream.h>
#include <qr/code.h>
#include <qr/data.h>
#include <qr/version.h>

struct config {
        int               version;
        enum qr_ec_level  ec;
        enum qr_data_type dtype;
        enum {
                FORMAT_ANSI,
                FORMAT_PBM,
                FORMAT_PNG
        } format;
        const char *      file;
        const char *      outfile;
        const char *      input;
};

struct qr_code * create(int               version,
                        enum qr_ec_level  ec,
                        enum qr_data_type dtype,
                        const char *      input,
                        size_t            len)
{
        struct qr_data * data;
        struct qr_code * code;

        data = qr_data_create(version, ec, dtype, input, len);

        if (!data) {
                /* BUG: this could also indicate OOM or
                 * some other error.
                 */
                fprintf(stderr, "Invalid data\n");
                exit(1);
        }

        code = qr_code_create(data);
        qr_data_destroy(data);

        if (!code) {
                perror("Failed to create code");
                exit(2);
        }

        return code;
}

void output_pbm(FILE * file, const struct qr_bitmap * bmp, const char * comment)
{
        unsigned char * row;
        int x, y;

        fputs("P1\n", file);

        if (comment)
                fprintf(file, "# %s\n", comment);

        fprintf(file, "%u %u\n",
               (unsigned)bmp->width + 8,
               (unsigned)bmp->height + 8);

        row = bmp->bits;

        for (y = -4; y < (int)bmp->height + 4; ++y) {

                if (y < 0 || y >= (int)bmp->height) {
                        for (x = 0; x < (int)bmp->width + 8; ++x)
                                fputs("0 ", file);
                        fputc('\n', file);
                        continue;
                }

                fputs("0 0 0 0 ", file);

                for (x = 0; x < (int)bmp->width; ++x) {

                        int mask = 1 << x % CHAR_BIT;
                        int byte = row[x / CHAR_BIT];

                        fprintf(file, "%c ", (byte & mask) ? '1' : '0');
                }

                fputs("0 0 0 0\n", file);
                row += bmp->stride;
        }
}

void output_ansi(FILE * file, const struct qr_bitmap * bmp)
{
        const char * out[2] = {
                "  ",
                "\033[7m  \033[0m",
        };

        unsigned char * line;
        size_t x, y;

        line = bmp->bits;

        for (y = 0; y < bmp->height; ++y) {

                for (x = 0; x < bmp->width; ++x) {

                        int mask = 1 << (x % CHAR_BIT);
                        int byte = line[x / CHAR_BIT];

                        fprintf(file, "%s", out[!!(byte & mask)]);
                }

                fputc('\n', file);

                line += bmp->stride;
        }
}

void output_png(FILE * file, const struct qr_bitmap * bmp, const char * comment)
{
        const int px_size = 4;
        png_structp png_ptr;
        png_infop info_ptr;
        png_text text;
        int x, y, p;
        unsigned char * row;

        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
                goto err;

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                goto err;

        if (setjmp(png_jmpbuf(png_ptr)))
                goto err;

        png_init_io(png_ptr, file);
        png_set_IHDR(png_ptr, info_ptr,
                (bmp->width + 8) * px_size,
                (bmp->height + 8) * px_size,
                1,
                PNG_COLOR_TYPE_GRAY,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);

        text.compression = PNG_TEXT_COMPRESSION_NONE;
        text.key = "Software";
        text.text = (char *) comment;
        text.text_length = strlen(comment);
        png_set_text(png_ptr, info_ptr, &text, 1);

        png_write_info(png_ptr, info_ptr);
        png_set_packing(png_ptr);

        row = malloc((bmp->width + 8) * px_size);
        memset(row, 1, (bmp->width + 8) * px_size);
        for (y = 0; y < 4; ++y)
                for (p = 0; p < px_size; ++p)
                        png_write_row(png_ptr, row);

        for (y = 0; y < bmp->height; ++y) {
                const unsigned char * bmp_row = bmp->bits + y * bmp->stride;
                unsigned char * out = row + 4 * px_size;
                for (x = 0; x < bmp->width; ++x) {
                        int px = bmp_row[x / CHAR_BIT] & (1 << (x % CHAR_BIT));
                        for (p = 0; p < px_size; ++p)
                                *out++ = !px;
                }
                for (p = 0; p < px_size; ++p)
                        png_write_row(png_ptr, row);
        }

        memset(row, 1, (bmp->width + 8) * px_size);
        for (y = 0; y < 4; ++y)
                for (p = 0; p < px_size; ++p)
                        png_write_row(png_ptr, row);

        free(row);
        png_write_end(png_ptr, info_ptr);
        png_destroy_write_struct(&png_ptr, NULL);
        return;

err:
        fprintf(stderr, "error writing PNG\n");
        exit(2);
}

void show_help() {
        fprintf(stderr,
                "Usage:\n\t%s [options] <data>\n\n"
                "\t-h         Display this help message\n"
                "\t-f <file>  File containing data to encode (- for stdin)\n"
                "\t-v <n>     Specify QR version (size) 1 <= n <= 40\n"
                "\t-e <type>  Specify EC type: L, M, Q, H\n"
                "\t-a         Output as ANSI graphics (default)\n"
                "\t-p         Output as PBM\n"
                "\t-g         Output as PNG\n"
                "\t-o <file>  File to write (- for stdout)\n\n",
                "qrgen");
}

void set_default_config(struct config * conf)
{
        conf->version = 0;
        conf->ec = QR_EC_LEVEL_M;
        conf->dtype = QR_DATA_8BIT;
        conf->format = FORMAT_ANSI;
        conf->file = NULL;
        conf->outfile = NULL;
        conf->input = NULL;
}

void parse_options(int argc, char ** argv, struct config * conf)
{
        int c;

        for (;;) {
                c = getopt(argc, argv, ":hf:v:e:t:apgo:");

                if (c == -1) /* no more options */
                        break;

                switch (c) {
                case 'h': /* help */
                        show_help();
                        exit(0);
                        break;
                case 'f': /* file */
                        conf->file = optarg;
                        break;
                case 'v': /* version */
                        conf->version = atoi(optarg);
                        if (conf->version < 1 || conf->version > 40) {
                                fprintf(stderr,
                                 "Version must be between 1 and 40\n");
                                exit(1);
                        }
                        break;
                case 'e': /* ec */
                        switch (tolower(optarg[0])) {
                        case 'l': conf->ec = QR_EC_LEVEL_L; break;
                        case 'm': conf->ec = QR_EC_LEVEL_M; break;
                        case 'q': conf->ec = QR_EC_LEVEL_Q; break;
                        case 'h': conf->ec = QR_EC_LEVEL_H; break;
                        default:
                                fprintf(stderr,
                                        "Invalid EC type (%c). Choose from"
                                        " L, M, Q or H.\n", optarg[0]);
                        }
                        break;
                case 't': /* type */
                        fprintf(stderr, "XXX: ignored \"type\"\n");
                        break;
                case 'a': /* ansi */
                        conf->format = FORMAT_ANSI; break;
                case 'p': /* pnm */
                        conf->format = FORMAT_PBM; break;
                case 'g': /* png */
                        conf->format = FORMAT_PNG; break;
                case 'o': /* output file */
                        conf->outfile = optarg; break;
                case ':':
                        fprintf(stderr,
                                "Argument \"%s\" missing parameter\n",
                                argv[optind-1]);
                        exit(1);
                        break;
                case '?': default:
                        fprintf(stderr,
                                "Invalid argument: \"%s\"\n"
                                "Try -h for help\n",
                                argv[optind-1]);
                        exit(1);
                        break;
                }
        }


        if (optind < argc)
                conf->input = argv[optind++];

        if (!conf->file && !conf->input) {
                fprintf(stderr, "No data (try -h for help)\n");
                exit(1);
        }
}

void slurp_file(const char * path, char ** data, size_t * len)
{
        const size_t chunk_size = 65536;
        FILE * file;
        char * tmpbuf;
        size_t count;

        if (strcmp(path, "-") == 0)
                file = stdin;
        else
                file = fopen(path, "rb");

        if (!file) {
                fprintf(stderr, "Failed to open %s\n", path);
                exit(2);
        }

        *data = NULL;
        *len = 0;

        do {
                tmpbuf = realloc(*data, *len + chunk_size);
                if (!tmpbuf) {
                        perror("realloc");
                        exit(2);
                }
                *data = tmpbuf;
                count = fread(*data + *len, 1, chunk_size, file);
                if (count == 0 && !feof(file)) {
                        perror("fread");
                        exit(2);
                }
                *len += count;
        } while (*len == chunk_size);

        fclose(file);
}

int main(int argc, char ** argv) {

        struct config conf;
        struct qr_code * code;
        char * file_data;
        size_t len;
        FILE * outfile;

        set_default_config(&conf);
        parse_options(argc, argv, &conf);

        if (conf.file)
                slurp_file(conf.file, &file_data, &len);
        else
                len = strlen(conf.input);

        if (!conf.outfile) {
                switch (conf.format) {
                case FORMAT_ANSI:
                        conf.outfile = "-"; break;
                case FORMAT_PBM:
                        conf.outfile = "qr.pbm"; break;
                case FORMAT_PNG:
                        conf.outfile = "qr.png"; break;
                }
        }

        if (strcmp(conf.outfile, "-") == 0) {
                outfile = stdout;
        } else {
                outfile = fopen(conf.outfile, "wb");
                if (!outfile) {
                        perror("fopen");
                        exit(2);
                }
        }

        code = create(conf.version,
                      conf.ec,
                      conf.dtype,
                      conf.file ? file_data : conf.input,
                      len);

        if (conf.file)
                free(file_data);

        switch (conf.format) {
        case FORMAT_ANSI:
                output_ansi(outfile, code->modules);
                break;
        case FORMAT_PBM:
                output_pbm(outfile, code->modules, "libqr v" QR_VERSION);
                break;
        case FORMAT_PNG:
                output_png(outfile, code->modules, "libqr v" QR_VERSION);
                break;
        }

        fclose(outfile);
        qr_code_destroy(code);

        return 0;
}

