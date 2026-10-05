#include <stdint.h>
#include <stdio.h>

/* The libraries' internal headers use overlapping type/macro names. Declare
 * their SAD table interfaces separately so both kernels can be called here. */
typedef int (*xeve_sad_fn)(int, int, void *, void *, int, int, int);
typedef int (*oapv_sad_fn)(int, int, void *, void *, int, int);
extern const xeve_sad_fn xeve_tbl_sad_16b_neon[8][8];
extern const oapv_sad_fn oapv_tbl_fn_sad_16b_neon[2];

static int reference_sad(const int16_t *a, const int16_t *b, int height)
{
    int sum = 0;
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < 8; ++x) {
            int difference = a[y * 17 + x] - b[y * 19 + x];
            sum += difference < 0 ? -difference : difference;
        }
    }
    return sum;
}

int main(void)
{
    int16_t a[19 * 128], b[19 * 128];
    for(int bit_depth = 8; bit_depth <= 12; bit_depth += 2) {
        int mask = (1 << bit_depth) - 1;
        for(int i = 0; i < 19 * 128; ++i) {
            a[i] = (int16_t)((i * 17 + 3) & mask);
            b[i] = (int16_t)((i * 31 + 5) & mask);
        }
        for(int log_height = 1; log_height <= 7; ++log_height) {
            int height = 1 << log_height;
            int expected = reference_sad(a, b, height) >> (bit_depth - 8);
            int actual = xeve_tbl_sad_16b_neon[3][log_height](8, height, a, b, 17, 19, bit_depth);
            if(actual != expected) {
                fprintf(stderr, "XEVE: depth=%d height=%d expected=%d actual=%d\n",
                        bit_depth, height, expected, actual);
                return 1;
            }
        }
        /* OpenAPV's pinned kernel operates on a fixed 8x8 block and does not
         * apply XEVE's bit-depth normalization. Check its own result too. */
        int expected = reference_sad(a, b, 8);
        int actual = oapv_tbl_fn_sad_16b_neon[0](8, 8, a, b, 17, 19);
        if(actual != expected) {
            fprintf(stderr, "OpenAPV: depth=%d expected=%d actual=%d\n", bit_depth, expected, actual);
            return 1;
        }
    }
    puts("ok: XEVE and OpenAPV static SAD kernels link together and return correct results");
    return 0;
}
