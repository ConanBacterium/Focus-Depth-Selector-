#include "minunit.h"
#include "../src/imgtools.h"


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

char *test_padImg() {
    int srcVals[] = {
        1,1,1,1,1,
        1,2,1,1,1,
        1,1,3,1,1,
        1,1,1,4,1,
        1,1,1,1,5
    };
    int tgtVals[] = {
        0,0,0,0,0,0,0,
        0,1,1,1,1,1,0,
        0,1,2,1,1,1,0,
        0,1,1,3,1,1,0,
        0,1,1,1,4,1,0,
        0,1,1,1,1,5,0,
        0,0,0,0,0,0,0
    };

    unsigned char *src = malloc(25);
    check_mem(src);

    for(int i = 0; i < 25; i++) {
        src[i] = (unsigned char) srcVals[i];
    }

    unsigned char *tgt = padImg(src, 5, 5, 0);

    /*
    printf("\n\n");
    for(int i = 0; i < 49; i++) {
        printf("%d,", tgt[i]);
        if(i%7==6) printf("\n");
    }
    */

    for(int i = 0; i < 49; i++) {
        mu_assert(tgt[i] == (unsigned char) tgtVals[i], "padImg failed to produce expected target!");
    }

    free(src);
    free(tgt);
    return NULL;

error: 
    return "non mu_assert error"; 
}

char *test_trimImg() {
    int srcVals[] = {
        0,0,0,0,0,0,0,
        0,1,1,1,1,1,0,
        0,1,2,1,1,1,0,
        0,1,1,3,1,1,0,
        0,1,1,1,4,1,0,
        0,1,1,1,1,5,0,
        0,0,0,0,0,0,0
    };
    int tgtVals[] = {
        1,1,1,1,1,
        1,2,1,1,1,
        1,1,3,1,1,
        1,1,1,4,1,
        1,1,1,1,5
    };
    unsigned char *src = malloc(49); 
    check_mem(src); 

    for(int i = 0; i < 49; i++) {
        src[i] = (unsigned char) srcVals[i];
    }

    unsigned char *tgt = trimImg(src, 7, 7);
    
    for(int i = 0; i < 25; i++) {
        mu_assert(tgt[i] == (unsigned char) tgtVals[i], "tgt not trimmed");
    }

    free(src); 
    free(tgt);
    return NULL; 
error: 
    return "non mu_assert error";
}

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
    memset(tgt, 9, 25); 
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

char *test_var() {
    int vals[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 12, 12, 15, 43, 52, 86, 94, 94, 105, 200, 254};
    double expectedVar = 4847.94;

    unsigned char *X = malloc(20);
    check_mem(X);
    for(int i = 0; i < 20; i++) {
        X[i] = vals[i];
    }

    double res = var(X, 20);
    printf("true: %f, calculated: %f\n", expectedVar, res);
    mu_assert(res == expectedVar, "var() didn't produce expected variance");

    return NULL;
error: 
    return "Test failed, memory error";
}

char *test_calcVol() {
    /*
    ../34/tape1/0/0.bmp has vol 800.022539
    ../34/tape1/0/0.bmp crop 0 has vol 800.022539
    ../34/tape1/0/0.bmp crop 1 has vol 918.871836
    ../34/tape1/0/0.bmp crop 2 has vol 886.939883
    ../34/tape1/0/0.bmp crop 3 has vol 868.839023
    ../34/tape1/0/0.bmp crop 4 has vol 819.913281
    ../34/tape1/0/0.bmp crop 5 has vol 783.328281
    ../34/tape1/0/0.bmp crop 6 has vol 861.329766
    ../34/tape1/0/0.bmp crop 7 has vol 866.458789
    ../34/tape1/0/0.bmp crop 8 has vol 763.240977
    ../34/tape1/0/0.bmp crop 9 has vol 709.356914
    ../34/tape1/0/0.bmp crop 10 has vol 742.474062
    ../34/tape1/0/0.bmp crop 11 has vol 921.403164
    ../34/tape1/0/0.bmp crop 12 has vol 849.191328
    ../34/tape1/0/0.bmp crop 13 has vol 838.561602
    ../34/tape1/0/0.bmp crop 14 has vol 868.200742
    ../34/tape1/0/0.bmp crop 15 has vol 789.478008
    ../34/tape1/0/0.bmp crop 16 has vol 793.346914
    ../34/tape1/0/0.bmp crop 17 has vol 684.134766
    ../34/tape1/0/0.bmp crop 18 has vol 627.725977
    ../34/tape1/0/0.bmp crop 19 has vol 575.836250
    ../34/tape1/0/0.bmp crop 20 has vol 571.259063
    ../34/tape1/0/0.bmp crop 21 has vol 523.551211
    ../34/tape1/0/0.bmp crop 22 has vol 508.818789
    ../34/tape1/0/0.bmp crop 23 has vol 512.985586
    */
    IMAGE *img = malloc(2560*1920);
    loadBnp(img, "../../34/tape1/0/0.bmp");
    double expectedVol = 800.022539;

    double vol = calcVol(img->imageData, 2560,1920);
    printf("\ntrue: %f, calculated: %f\n", expectedVol, vol);
    mu_assert(vol==expectedVol, "calcVol doesn't produce correct vol of tape1 img 0");

    return NULL;
}

char *all_tests() {
    mu_suite_start();

    mu_run_test(test_loadBnp_destroyImage);
    mu_run_test(test_switchLineOrder);
    mu_run_test(test_var);
    mu_run_test(test_padImg);
    mu_run_test(test_trimImg);
    mu_run_test(test_laplacianTransform);
    // mu_run_test(test_calcVol);

    return NULL;
}

RUN_TESTS(all_tests);