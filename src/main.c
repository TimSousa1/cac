#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define PPM_LIB_IMPL_TIM
#include "ppm.h"

enum Error {
    IO_ERR=1,
    MALLOC_ERR,
};

typedef struct {
    int32_t w;
    int32_t h;
    u_int8_t *pixels;
} Image;

int main(void) {
    int32_t err;
    char input_filename[] = "in.ppm";

    FILE *input_file = NULL,
         *output_file = NULL;

    Image input, output;

    int32_t bitwidth = -1,
        magic_type = -1;

    input.pixels = NULL;
    input_file = fopen(input_filename, "r");
    if (!input_file) {
        fprintf(stderr, "ERROR %d: couldn't open %s.. is the file missing?\n", IO_ERR, input_filename);
        return IO_ERR;
    }

    err = read_ppm_header(input_file, &input.w, &input.h, &bitwidth, &magic_type);
    if (err) {
        fclose(input_file);

        print_ppm_error(err);
        return 0;
    }

    input.pixels = malloc(input.w*input.h*3);
    if (!input.pixels) {
        fclose(input_file);
        return MALLOC_ERR;
    }

    err = read_ppm(input.pixels, input_file, input.w, input.h, bitwidth, magic_type);
    if (err) {
        free(input.pixels);
        fclose(input_file);

        print_ppm_error(err);
        return 0;
    }
    // PROCESSING

    /* TO BE DONE */

    // WRITING
    output_file = fopen("out.ppm", "w");
    if (!output_file) {
        free(input.pixels);
        fclose(input_file);

        return IO_ERR;
    }

    err = write_ppm_p6(input.pixels, output_file, input.w, input.h, bitwidth);
    if (err) {
        free(input.pixels);
        fclose(input_file);
        fclose(output_file);

        print_ppm_error(err);
    }

    return 0;
}
