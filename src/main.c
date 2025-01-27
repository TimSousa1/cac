#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define PPM_LIB_IMPL_TIM
#include "ppm.h"
#include "entropy.h"

enum Error {
    IO_ERR=1,
	ARG_ERR,
    MALLOC_ERR,
    CALLOC_ERR,
	PPM_ERR,
};

typedef struct {
    uint32_t w;
    uint32_t h;
    u_int8_t *pixels;
} Image;

void usage(void) {
	printf("Usage: /path/to/cac [options] file\n");
	printf("Options:\n");
	printf("\t-h\t\t\tDisplays this information\n");
	printf("\t-o <outfile>\t\tChanges the output file name. Defaults to \"out.ppm\"\n");
}

int main(int argc, char **argv) {

    int32_t err,
			c;
	
    char *input_filename = NULL,
		 *output_filename = NULL,
		 default_out_name[] = "out.ppm";

    FILE *input_file = NULL,
         *output_file = NULL;

    Image input, output;

    int32_t bitwidth = -1,
		    magic_type = -1;

	output_filename = default_out_name;

	while (c = getopt(argc, argv, "ho:"), c != -1) {
		switch (c) {
			case 'h':
				usage();
				return 0;
			case 'o':
				output_filename = optarg;
				break;
			default:
				usage();
				return ARG_ERR;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Missing input file name.\n");
		usage();
		return ARG_ERR;
	}
	
	input_filename = argv[optind];

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
        return PPM_ERR;
    }

    input.pixels = malloc(input.w*input.h*3);
    if (!input.pixels) {
        fclose(input_file);
        return MALLOC_ERR;
    }

    err = read_ppm(input.pixels, input_file, input.w, input.h, bitwidth, magic_type);
    if (err) {
        fclose(input_file);
		free(input.pixels);

        print_ppm_error(err);;
        return PPM_ERR;
    }

    fclose(input_file);
    // PROCESSING

    // entropy calc
    int32_t *entropy = calloc(input.w*input.h, sizeof(*entropy));
    if (!entropy) {
		free(input.pixels);

        return CALLOC_ERR;
    }

    int32_t max, min;
    FILE *entropy_img = fopen("entropy.ppm", "w");
    if (!entropy_img) {
		free(input.pixels);
        free(entropy);

        return CALLOC_ERR;
    }

    compute_entropy_edge(entropy, &max, &min, input.pixels, input.w, input.h, 0); // vertically
    compute_entropy_edge(entropy, &max, &min, input.pixels, input.w, input.h, 1); // horizontally
    err = write_entropy(entropy, entropy_img, input.w, input.h, min, max);
    if (err) printf("entropy err?\n");

    fclose(entropy_img);

    // removing pixels
    uint32_t N = 1000; // should be taken as an argument
    uint32_t **to_remove = malloc(N * sizeof(*to_remove));
    if (!to_remove) {
        free(input.pixels);
        free(entropy);
        return MALLOC_ERR;
    }

    for (uint32_t i = 0; i < N; i++) {
        to_remove[i] = malloc(input.h * sizeof(*to_remove[i]));
        if (!to_remove[i]) {
            for (int32_t j = (int32_t)(i-1); j >= 0; j--) {
                printf("%d ", j);
                free(to_remove[j]);
            }

            free(input.pixels);
            free(entropy);
            free(to_remove);
            return MALLOC_ERR;
        }
    }

    for (uint32_t n = 0; n < N; n++) {
        for (uint32_t y = 0; y < input.h; y++) {
            to_remove[n][y] = n*2;
        }
    }

    FILE *cropped = fopen("cropped.ppm", "w");
    write_cropped(input.pixels, to_remove, cropped, input.w, input.h, N);


    // WRITING
    output_file = fopen(output_filename, "w");
    if (!output_file) {
        free(input.pixels);
        free(entropy);

        for (uint32_t i = 0; i < N; i++) free(to_remove[i]);
        free(to_remove);

        return IO_ERR;
    }

    err = write_ppm_p6(input.pixels, output_file, input.w, input.h, bitwidth);
    if (err) {
        fclose(output_file);

        free(input.pixels);
        free(entropy);

        for (uint32_t i = 0; i < N; i++) free(to_remove[i]);
        free(to_remove);

        print_ppm_error(err);
        return PPM_ERR;
    }

    for (uint32_t i = 0; i < N; i++) free(to_remove[i]);
    free(to_remove);
    fclose(output_file);

    free(input.pixels);
    free(entropy);
    return 0;
}
