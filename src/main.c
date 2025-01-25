#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>

#include <sys/types.h>

#define PPM_LIB_IMPL_TIM
#include "ppm.h"

enum Error {
    IO_ERR=1,
	ARG_ERR,
    MALLOC_ERR,
};

typedef struct {
    int32_t w;
    int32_t h;
    u_int8_t *pixels;
} Image;

void usage(void) {
	printf("Usage: /path/to/cac [options] file\n");
	printf("Options:\n");
	printf("\t-h\t\t\tDisplays this information\n");
	printf("\t-o <outfile>\t\tChanges the output file name. Defaults to <inputfile>_out.ppm\n");
}

int main(int argc, char **argv) {

    int32_t err,
			size,
			c,
			input_index;
	
    char *input_filename = NULL,
		 *input_arg = NULL,
		 *output_filename = NULL;

    FILE *input_file = NULL,
         *output_file = NULL;

    Image input, output;

    int32_t bitwidth = -1,
		    magic_type = -1;

	while (c = getopt(argc, argv, "ho:"), c != -1) {
		switch (c) {
			case 'h':
				usage();
				return 0;
			case 'o':
				size = strlen(optarg);
				output_filename = malloc(size + 1);
				output_filename = strcpy(output_filename, optarg);
				break;
			case '?':
				if (optopt == 'o')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				if (output_filename) free(output_filename);
				usage();
				return ARG_ERR;
			default:
				if (output_filename) free(output_filename);
				usage();
				return ARG_ERR;
		}
	}
	if (input_index = optind, input_index >= argc) {
		fprintf(stderr, "Missing input file name.\n");
		if (output_filename) free(output_filename);
		usage();
		return ARG_ERR;
	}
	
	input_arg = argv[input_index];
	size = strlen(input_arg);
	input_filename = malloc(size + 1);
	input_filename = strcpy(input_filename, input_arg);

	if (!output_filename) {
		size = strlen(input_filename);
		output_filename = malloc(size + 4 + 1);
		output_filename = strncpy(output_filename, input_filename, size - 4);
		output_filename[size - 4] = '\0';
		output_filename = strcat(output_filename, "_out.ppm");
	}

	printf("%s\n", output_filename);

	input.pixels = NULL;
    input_file = fopen(input_filename, "r");
    if (!input_file) {
        fprintf(stderr, "ERROR %d: couldn't open %s.. is the file missing?\n", IO_ERR, input_filename);
		free(input_filename);
		free(output_filename);
        return IO_ERR;
    }
	free(input_filename);

    err = read_ppm_header(input_file, &input.w, &input.h, &bitwidth, &magic_type);
    if (err) {
        fclose(input_file);
		free(output_filename);
        
		print_ppm_error(err);
        return 0;
    }

    input.pixels = malloc(input.w*input.h*3);
    if (!input.pixels) {
        fclose(input_file);
		free(output_filename);
        return MALLOC_ERR;
    }

    err = read_ppm(input.pixels, input_file, input.w, input.h, bitwidth, magic_type);
    if (err) {
        free(input.pixels);
        fclose(input_file);
		free(output_filename);

        print_ppm_error(err);;
        return 0;
    }
    // PROCESSING

    /* TO BE DONE */

    // WRITING
    output_file = fopen(output_filename, "w");
	free(output_filename);
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

	free(input.pixels);
	fclose(input_file);
	fclose(output_file);
    return 0;
}
