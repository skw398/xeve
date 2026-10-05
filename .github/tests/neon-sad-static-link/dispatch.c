#include <stdint.h>
#include <stdio.h>

typedef int (*xeve_sad_fn)(int, int, void *, void *, int, int, int);
extern const xeve_sad_fn xeve_tbl_sad_16b_neon[8][8];

/* Exercise all five NEON helpers through their existing dispatch entries.
 * Comparing with unmodified XEVE isolates the effect of the linkage change. */
int main(void)
{
    int16_t a[139 * 128], b[139 * 128];
    for(int bit_depth = 8; bit_depth <= 12; bit_depth += 2) {
        int mask = (1 << bit_depth) - 1;
        for(int i = 0; i < 139 * 128; ++i) {
            a[i] = (int16_t)((i * 17 + 3) & mask);
            b[i] = (int16_t)((i * 31 + 5) & mask);
        }
        for(int log_width = 0; log_width <= 7; ++log_width) {
            for(int log_height = 0; log_height <= 7; ++log_height) {
                int width = 1 << log_width;
                int height = 1 << log_height;
                int result = xeve_tbl_sad_16b_neon[log_width][log_height](
                    width, height, a, b, 137, 139, bit_depth);
                printf("depth=%d width=%d height=%d sad=%d\n", bit_depth, width, height, result);
            }
        }
    }
    return 0;
}
