#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "entropy.h"
#include "ppm.h"

void extract_entropy(int32_t *entropy, uint32_t w, uint32_t h) {
    printf("int32_t entropy[] = {");
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            printf("%3d, ", entropy[Cidx(x, y, w)]);
        }
        printf("\n");
    }
    printf("};\n");
}

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

// hasn't been tested in a while
int write_entropy_p3(int32_t *entropy_levels, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max, uint32_t b) {

    if (max == min) return ENTROPY_REDUNDANT_WRITE;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P3", w, h, b);

    uint32_t i = 0;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            const uint8_t col = (uint8_t) Ccol(entropy_levels[i], max, min, (int32_t)b);
            fprintf(img, "%u %u %u ", col,col,col);
            i++;
        }
        fprintf(img, "\n");
    }


    return 0;
}

int write_entropy_to_remove(int32_t *entropy, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max, uint32_t b, int32_t **to_remove) {

    if (max == min) return ENTROPY_REDUNDANT_WRITE;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P6", w, h, b);

    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            const uint32_t idx = Cidx(x, y, w);
            uint8_t col = (uint8_t) Ccol(entropy[idx], max, min, (int32_t) b);
            if (x == to_remove[0][y]) col = b;
            fwrite(&col, 1, 1, img);

            if (x == to_remove[0][y]) col = 0;
            fwrite(&col, 1, 1, img);
            fwrite(&col, 1, 1, img);
        }
    }

    return 0;
}

int write_entropy(int32_t *entropy_levels, FILE *img, uint32_t w, uint32_t h, int32_t min, int32_t max, uint32_t b, uint32_t mode) {

    if (mode == 1) min = 0;
    if (max == min) return ENTROPY_REDUNDANT_WRITE;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P6", w, h, b);

    int32_t n_bytes;
    for (uint32_t i = 0; i < w*h*3; i++) {
        int64_t cost = entropy_levels[i/3];
        if (mode == 1) cost *= cost;
        const uint8_t col = (uint8_t) Ccol(cost, (int64_t) (max*max), 0, (int64_t)b) ;
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


int srt(const void *a, const void *b) {
    int _a = *(int*) a;
    int _b = *(int*) b;
    return  (_a > _b);
}

int write_cropped(uint8_t *pixels, int32_t **to_remove, FILE *img, uint32_t w, uint32_t h, uint32_t N, uint32_t b) {

    if (!img) return ENTROPY_IO_ERR;

    rewind(img);
    fprintf(img, "%s\n%d %d\n%d\n", "P6", w-N, h, b);

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

typedef struct _cost_node {
    uint32_t x;
    uint32_t y;
    int64_t cost;
    bool visited;
    struct _cost_node *came_from;
} c_node;


int srt_cost(const void *a, const void *b) {
    c_node **_a = (c_node**) a;
    c_node **_b = (c_node**) b;
    return ((*_a)->cost < (*_b)->cost);
}


int remove_pixels(int32_t *entropy, int32_t **to_remove, uint32_t w, uint32_t h, int start, uint32_t N){
    // allocating grid
    c_node *nodes = calloc(w*(h+1), sizeof(*nodes)); // last line is finishing line
    c_node **queue = calloc(w*h, sizeof(*queue));    // queue of items to visit
    // bool *removed = calloc(w*h, sizeof(*removed));   // bitmap of already removed items

    // possible moves
    int32_t moves[] = {-1, 0, 1};

    c_node *cur = NULL;

    for (uint32_t n = 0; n < N; n++, start++) {
        printf("n: %u\n", 0);
        // start
        memset(nodes, -1, w*h*sizeof(*nodes));
        memset(queue, 0, w*h*sizeof(*queue)); // already calloc'd

        int32_t q_el = 0; // index of element to be added

        for (uint32_t i = 0; i < w*h; i++) {
            nodes[i].visited = 0;
            nodes[i].came_from = NULL;
        }

        // initializing finishing line
        for (uint32_t x = 0; x < w; x++) {
            c_node *tmp = &nodes[Cidx(x, h, w)];
            tmp->visited = 0;
            tmp->cost = 0; // WRONG
            tmp->came_from = 0;
            tmp->x = x;
            tmp->y = h;
        }

        // starting node
        c_node root;
        root.x = start;
        root.y = 0;
        root.came_from = NULL;

        queue[0] = &root;
        q_el++;

        cur = queue[0];
        q_el--;

        // uint64_t it = 0;

        do {
            // printf("%lu\n", it);
            // it++;
            // if (cur->y < h) removed[Cidx(cur->x, cur->y, w)] = 1;
            cur->visited = 1;
            c_node *to_add[3] = {0}; // pointers to valid neighbours
            uint32_t added = 0;      // number of heighbours added

            // check all possible moves
            for (uint32_t m = 0; m < 3; m++) {
                int32_t new_x = cur->x + moves[m];
                int32_t new_y = cur->y+1;

                // seems like these lines make the program go crazy..
                if (new_y > (int32_t)h+1) goto skip_for;
                for (uint32_t ni = 0; ni < n; ni++) {
                    if (new_x >= to_remove[ni][new_y]) {
                        new_x++;
                    }
                }
skip_for:

                // skip if out of bounds
                if (!is_in_bounds(new_x, new_y, (int32_t)w, (int32_t)(h+1))) continue;
                // if (new_x < 0 || new_x >= w || new_y < 0 || new_y > h) continue;

                // while (removed[Cidx(new_x, new_y, w)] && (new_x+(m==1)*1 + (moves[m]) >= 0) && (new_x+(m==1)*1 + (moves[m]) < w)) new_x+= (m == 1)*1 + (moves[m]);


                // create cost node
                c_node *new = &nodes[Cidx(new_x, new_y, w)];

                if (new->visited) continue; // we already know the fastest route

                new->x = new_x;
                new->y = new_y;

                int64_t edge_cost = (int64_t)(/*entropy[Cidx(new_x, new_y, w)]*/entropy[Cidx(new_x, new_y, w)]);
                edge_cost *= edge_cost; // no negatives allowed
                                        // if cost < 0: the node hasn't been visited and it's cost should be updated
                                        // if not visited, the node should be updated
                                        // if it's cost is bigger than the one we found, update it accordingly
                                        //   and where we came from too

                if (new->cost < 0 || new->cost > cur->cost + edge_cost) {
                    new->cost = cur->cost + edge_cost;
                    new->came_from = cur;

                    // add to to_add list, increment number of added
                    to_add[added] = new;
                    added++;
                }
            }


            // add elements to queue
            for (uint32_t i = 0; i < added; i++) {
                queue[q_el] = to_add[i];
                q_el++;
            }

            // get element with the shortest path from start
            uint32_t min = 0;
            c_node *min_cost = queue[0];
            for (int32_t i = 0; i < q_el; i++) {
                if (queue[i]->cost < min_cost->cost) {
                    min = i;
                    min_cost = queue[i];
                }
            }

            c_node *tmp = cur;
            cur = queue[min];
            queue[min] = queue[q_el-1]; // swap last to next
            q_el--;

            if (cur->y == h) break;

        } while(q_el > 0);

        // get bottom element with least cost
        c_node *least_cost = &nodes[Cidx(0, h-1, w)];
        int32_t min = entropy[Cidx(0, h-1, w)];
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = Cidx(x, h-1, w);
            if (entropy[idx] < min) {
                min = entropy[idx];
                least_cost = &nodes[idx];
            }
        }

        // print found path and add to list
        uint32_t y = h-1;
        for (c_node *node = least_cost; node; node = node->came_from) {
            printf("<-%d,%d (%ld)", node->x, node->y, node->cost);
            // removed[Cidx(node->x, node->y, w)] = 1;
            to_remove[n][y] = (int32_t) node->x;
            y--;
        }
        // printf("\n");
    }


    free(nodes);
    free(queue);
    return 0;
}


int write_to_remove(FILE *img, uint8_t *pixels, int32_t **to_remove, uint32_t w, uint32_t h, uint32_t b, uint32_t N) {
    uint8_t *removed_pixels = malloc(w*h*3*sizeof(*removed_pixels));
    memcpy(removed_pixels, pixels, w*h*3*sizeof(*pixels));

    for (uint32_t n = 0; n < N; n++) {
        for (uint32_t y = 0; y < h; y++) {
            // printf("%d,%d ", to_remove[0][y], y);

            removed_pixels[3*Cidx(to_remove[n][y], y, w)+0] = b;
            removed_pixels[3*Cidx(to_remove[n][y], y, w)+1] = 0;
            removed_pixels[3*Cidx(to_remove[n][y], y, w)+2] = 0;
        }
    }

    write_ppm_p6(removed_pixels, img, w, h, b);

    return 0;
}

