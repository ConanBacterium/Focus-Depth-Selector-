#include "minunit.h"
#include "../src/imgtools.h"

char *test_laplacianTransform(){
    // src img too small 
    unsigned char *src = malloc(16);
    unsigned char *tgt = malloc(16);
    unsigned char *expected = malloc(25); 
    check_mem(src);
    check_mem(tgt);
    check_mem(expected);
    int rc  = laplacianTransform(src, tgt, 4, 4);
    mu_assert(rc != 0, "laplacianTransform rc != 0");

    // src is all 1s 
    src = malloc(25);
    tgt = malloc(25);
    memset(src, 1, 25);
    memset(tgt, 9, 25); // differs from expected
    int expectedVals[] = {9,9,9,9,9,
                        9,0,0,0,9,
                        9,0,0,0,9,
                        9,0,0,0,9,
                        9,9,9,9,9};
    for (int i = 0; i < 25; i++) {
       expected[i] = (unsigned char) expectedVals[i]; 
    }
    rc = laplacianTransform(src, tgt, 5, 5);
    mu_assert(rc==0, "laplacianTransform failed");
    rc = memcmp(tgt, expected, 25);
    mu_assert(rc==0, "src of 1s doesn't produce tgt of 0s");

/*
        // bigger but still only 4x4 with border
    int srcVals[] = { 
                    0,0,0,0,0,
                    0,1,2,3,0,
                    0,4,5,6,0,
                    0,9,7,8,0,
                    0,0,0,0,0
                    };
    int expectedVals1[] = { 
                    0,0,0,0,0,
                    0,2,17,20,0,
                    0,31,39,40,0,
                    0,47,50,45,0,
                    0,0,0,0,0
                    };
    for(int i = 0; i < 25; i++) {
        src[i] = (unsigned char) srcVals[i];
        expected[i] = (unsigned char) expectedVals1[i];
    }
    rc = laplacianTransform(src, tgt, 5, 5);
    printf("\n");
    for(int i = 0; i < 25; i+=5) {
        printf("%d,%d,%d,%d,%d",tgt[i],tgt[i+1],tgt[i+2],tgt[i+3],tgt[i+4]);
        printf("\n");
    }
    mu_assert(rc==0, "laplacianTransform failed");
    rc = memcmp(tgt, expected, 25);
    mu_assert(rc==0, "laplacian failed on non-universal array");
*/

    free(src);
    free(tgt);
    free(expected);
    return NULL; 

error: 
    free(src);
    free(tgt);
    free(expected);
    return "smth went wronk that wasn't mu_assert";
}

char *test_loadBnp_destroyImage() {
    IMAGE *img = malloc(sizeof(IMAGE));
    check_mem(img);
    int rc = loadBnp(img, "/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/c-skeleton/tests/1.bmp");
    mu_assert(rc==0, "failed to load img"); 

    rc = destroyImage(img);
    mu_assert(rc==0, "failed to destroy img"); 

    return NULL; 
error: 
    return "memory error in test, not fault of subjects of tests";
}

char *test_switchLineOrder() {
    // IMPORTANT! this test won't work if in future switchLineOrder requires proper INFOHEADER and FILEHEADER
    // in general I would say this test is not very good, but w/e
    IMAGE *img = malloc(sizeof(IMAGE));
    check_mem(img);

    img->imageData = malloc(25);
    check_mem(img->imageData); 

    int imgData[] = {
        1,1,1,1,1,
        2,2,2,2,2,
        3,3,3,3,3,
        4,4,4,4,4,
        5,5,5,5,5
    };

    for(int i = 0; i < 25; i++) {
        img->imageData[i] = (unsigned char) imgData[i];
    }

    INFOHEADER dummyHeader = {
        .biSize = sizeof(INFOHEADER),
        .width = 5,
        .height = 5,
        .planes = 1,             // Typically 1 for BMP
        .bitPix = 8,            // Assuming a 24-bit color depth (can change as needed)
        .biCompression = 0,      // No compression
        .biSizeImage = 5 * 5, // Width * Height * (Bit depth / 8), assuming 24 bits per pixel
        .biXPelsPerMeter = 0,    // Resolution not specified
        .biYPelsPerMeter = 0,    // Resolution not specified
        .biClrUsed = 0,          // Number of colors used (0 for default)
        .biClrImportant = 0      // All colors are important
    };

    img->ih = dummyHeader;

    int rc = switchLineOrder(img);
    mu_assert(rc == 0, "return code not zero (switchLineOrder)")

    for(int i = 0; i < 25; i++) {
        // note: this is only valid because the lines consist of the same data
        // printf("\n%d-%d\n", img->imageData[i], imgData[24-i]);
        mu_assert(img->imageData[i] == (unsigned char) imgData[24-i], "switchLineOrder didn't reverse");
    }

    free(img->imageData);
    free(img);

    return NULL;
error: 
    return "error not caused by mu_assert";
}

char *all_tests() {
    mu_suite_start();

    mu_run_test(test_laplacianTransform);
    mu_run_test(test_loadBnp_destroyImage);
    mu_run_test(test_switchLineOrder);

    return NULL;
}

RUN_TESTS(all_tests);