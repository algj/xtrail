#pragma once

#include <immintrin.h>
#include <string.h>

#include "./global.c"
#include "./point.c"

/*
This is used to keep track of the area that has been used for rendering, so it could be cleaned up afterwards.
E.g., if Point is written to, it will enlarge the area that will have to be cleaned up.
*/

void inline renderAreaReset(RenderArea *a, int width, int height) {
    a->prev_x1 = a->x1;
    a->prev_y1 = a->y1;
    a->prev_x2 = a->x2;
    a->prev_y2 = a->y2;
    a->x1 = width;
    a->y1 = height;
    a->x2 = a->y2 = 0;
    a->wasClean = a->isClean;
    a->isClean = 1;
}

void inline renderAreaClaim(RenderArea *a, Point p) {
    a->isClean = 0;
    a->x1 = MIN(a->x1, p.x);
    a->y1 = MIN(a->y1, p.y);
    a->x2 = MAX(a->x2, p.x);
    a->y2 = MAX(a->y2, p.y);
}

// this is a bit crazy, but oh well
char inline is_avx2_supported() {
#ifdef __GNUC__  // Check if using GCC or compatible compiler
    unsigned int eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0) :);
    return (ebx & (1 << 5)) != 0;
#else
    // Add code for other compilers
    // You may need to use compiler-specific intrinsics or functions for other compilers
    return false;  // Assume AVX2 is not supported on non-GCC compilers
#endif
}
void inline renderAreaClearImage(RenderArea *a, XImage *image) {
    int x = MAX(0, a->x1-1);
    int y = MAX(0, a->y1-1);
    int width = MIN(a->x2, image->width - 1) - a->x1 + 1;
    int height = MIN(a->y2, image->height - 1) - a->y1 + 1;

    // check if AVX2 is supported
    if (is_avx2_supported()) {
        const int alignment = 32;     // assuming 32-byte alignment
        const int pixelsPerByte = 8;  // pixels per byte in a 1-bit depth image

        int bytesPerLine = (width + pixelsPerByte - 1) / pixelsPerByte;  // Rounded up division

        int simdOperationsPerLine = bytesPerLine / (alignment / 8);  // Convert alignment to bytes

        for (int i = 0; i < height; ++i) {
            char *lineStart = image->data + (y + i) * image->bytes_per_line + (x / pixelsPerByte);
            _mm_prefetch((char *)lineStart, _MM_HINT_T0);

            // unroll the inner loop for better performance
            for (int j = 0; j < simdOperationsPerLine; j += 2) {
                // use SIMD intrinsics to set bytes to zero
                _mm_store_si128((__m128i *)(lineStart + j * (alignment / 8)), _mm_setzero_si128());
                _mm_store_si128((__m128i *)(lineStart + (j + 1) * (alignment / 8)), _mm_setzero_si128());
            }
        }
    } else {
        // Fallback to non-SIMD version if SIMD is not supported or alignment is not suitable
        for (int i = 0; i < height; ++i) {
            memset(image->data + (y + i) * image->bytes_per_line + x / 8, 0, (width + 15) / 8);
        }
    }
}