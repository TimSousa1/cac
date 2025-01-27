#include <stdio.h>
#include <stdint.h>

#define Cidx(x, y, maxx) ((y)*(maxx)+(x))
#define Ccol(val, max, min) ( ((val)-(min)) * 255/((max)-(min)) )
#define is_in_bounds(x, y, maxx, maxy) ((x) > 0 && (x) < (maxx) && (y) > 0 && (y) < (maxy))

enum Entropy_error {
    ENTROPY_IO_ERR=1,
    ENTROPY_REDUNDANT_WRITE
};


// returns the entropy values, the max and min 
// and takes as arguments the width, height and the pixels of the original image
int compute_entropy(int32_t *entropy_levels, int32_t *max, int32_t *min,  uint8_t *pixels, uint32_t w, uint32_t h);

// writes the entropy levels to a file
int write_entropy(int32_t *entropy_levels, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max);
int write_entropy_p3(int32_t *entropy_levels, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max);

int compute_entropy_edge(int32_t *entropy_levels, int32_t *max, int32_t *min,  uint8_t *pixels, uint32_t w, uint32_t h, uint8_t mode);
int write_cropped(uint8_t *pixels, uint32_t **to_remove, FILE *img, uint32_t w, uint32_t h, uint32_t N);
