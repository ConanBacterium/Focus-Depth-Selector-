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
    IMAGE img;
    int rc = loadBnp(&img, "1.bmp");
    mu_assert(rc==0, "failed to load img"); 

    rc = destroyImage(&img);
    mu_assert(rc==0, "failed to destroy img"); 

    return NULL; 
}

char *all_tests() {
    mu_suite_start();

    mu_run_test(test_laplacianTransform);

    return NULL;
}

RUN_TESTS(all_tests);