#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "entropy.h"


// returns the entropy values, the max and min 
// and takes as arguments the width, height and the pixels of the original image
int compute_entropy(int32_t *entropy_levels, int32_t *max, int32_t *min,  uint8_t *pixels, uint32_t w, uint32_t h) {

    *max = INT32_MIN, *min = INT32_MAX;
    for (uint32_t i = 0; i < w*h; i++) {
        int32_t val;
        val = pixels[i*3 + 0] + pixels[i*3 + 1] + pixels[i*3 + 2];
        val /= 3;
        entropy_levels[i] = val;

        *max = (*max < val) ? val : *max;
        *min = (*min > val) ? val : *min;
    }

    return 0;
}

int write_entropy_p3(int32_t *entropy_levels, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max) {

    if (max == min) return ENTROPY_REDUNDANT_WRITE;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P3", w, h, 255);

    uint32_t i = 0;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            const uint8_t col = (uint8_t) Ccol(entropy_levels[i], max, min);
            fprintf(img, "%u %u %u ", col,col,col);
            i++;
        }
        fprintf(img, "\n");
    }


    return 0;
}

int write_entropy(int32_t *entropy_levels, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max) {

    if (max == min) return ENTROPY_REDUNDANT_WRITE;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P6", w, h, 255);

    int32_t n_bytes;
    for (uint32_t i = 0; i < w*h*3; i++) {
        const uint8_t col = (uint8_t) Ccol(entropy_levels[i/3], max, min);
        n_bytes = fwrite(&col, 1, 1, img);

        if (n_bytes != 1) return ENTROPY_IO_ERR;
    }

    return 0;
}

// adds entropy to *entropy_levels
int compute_entropy_edge(int32_t *entropy_levels, int32_t *max, int32_t *min,  uint8_t *pixels, uint32_t w, uint32_t h, uint8_t mode) {
    const uint32_t ew = 3, eh = 3;

    const float edge_matrix_vertical[] = {-1,  0, 1,
                                          -1,  0, 1,
                                          -1,  0, 1,};

    const float edge_matrix_horizontal[] = {-1, -1, -1,
                                             0,  0,  0,
                                             1,  1,  1,};

    float *edge_matrix = edge_matrix_vertical;
    if (mode == 0) 
        edge_matrix = edge_matrix_vertical;
    if (mode == 1) 
        edge_matrix = edge_matrix_horizontal;


    *max = INT32_MIN, *min = INT32_MAX;

    for (uint32_t x = 0; x < w; x++) {
        for (uint32_t y = 0; y < h; y++) {

            float val = 0;
            for (uint32_t ey = 0; ey < eh; ey++) {
                for (uint32_t ex = 0; ex < ew; ex++) {
                    if (!is_in_bounds(x+ex-1, y+ey-1, w, h)) continue;

                    val += edge_matrix[Cidx(ex, ey, ew)] * (pixels[3*Cidx(x+ex-1, y+ey-1, w)] + pixels[3*Cidx(x+ex-1, y+ey-1, w)+1] + pixels[3*Cidx(x+ex-1, y+ey-1, w)+2]);
                }
            }

            entropy_levels[Cidx(x, y, w)] += val;
            *max = (*max < val) ? val : *max;
            *min = (*min > val) ? val : *min;

        }
    }

    return 0;
}

int _compute_entropy_edge(int32_t *entropy_levels, int32_t *max, int32_t *min,  uint8_t *pixels, uint32_t w, uint32_t h) {

    const uint32_t ew = 3, eh = 3;
    const float edge_matrix[] = { 1,  1,  1,
                                  1,  1,  1,
                                  1,  1,  1};
    
    *max = INT32_MIN, *min = INT32_MAX;
    for (uint32_t y = 0; y < h -eh+1; y++) {
        for (uint32_t x = 0; x < w -ew+1; x++) {

            float val = 0;
            for (uint32_t ey = 0; ey < eh; ey++) {
                for (uint32_t ex = 0; ex < ew; ex++) {
                    val += edge_matrix[Cidx(ex, ey, ew)] * (pixels[3*Cidx(x+ex, y+ey, w)] + pixels[3*Cidx(x+ex, y+ey, w)+1] + pixels[3*Cidx(x+ex, y+ey, w)+2]);
                }
            }

            entropy_levels[Cidx(x+1, y+1, w)] = (int32_t) val;
            *max = (*max < val) ? val : *max;
            *min = (*min > val) ? val : *min;
        }
    }

    return 0;
}

int srt(const void *a, const void *b) {
    int _a = *(int*) a;
    int _b = *(int*) b;
    return  (_a > _b);
}

int write_cropped(uint8_t *pixels, uint32_t **to_remove, FILE *img, uint32_t w, uint32_t h, uint32_t N) {

    if (!img) return ENTROPY_IO_ERR;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P6", w-N, h, 255);

    uint32_t *buf = malloc(N*sizeof(*buf));
    int32_t n_bytes = -1;

    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t n = 0; n < N; n++) {
            buf[n] = to_remove[n][y];
        }

        qsort(buf, N, sizeof(*buf), srt);

        n_bytes = fwrite(pixels, sizeof(*pixels), (buf[0])*3, img);
        if (n_bytes != (int32_t) (buf[0]*3*sizeof(*pixels)))
            return ENTROPY_IO_ERR;

        pixels += (buf[0]+1)*3;
        for (uint32_t n = 1; n < N; n++) {
            n_bytes = fwrite(pixels, sizeof(*pixels), (buf[n]-buf[n-1]-1)*3, img);

            if (n_bytes != (int32_t)((buf[n]-buf[n-1]-1)*3*sizeof(*pixels))) 
                return ENTROPY_IO_ERR;

            pixels += (buf[n]-buf[n-1])*3;
        }

        n_bytes = fwrite(pixels, sizeof(*pixels), (w-buf[N-1]-1)*3, img);

        if (n_bytes != (int32_t) ((w-buf[N-1]-1)*3*sizeof(*pixels)))
            return ENTROPY_IO_ERR;

        pixels += (w-buf[N-1]-1)*3;
    }

    free(buf);
    return 0;
}

