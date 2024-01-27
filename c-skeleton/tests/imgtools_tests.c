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
    int *paddedTgt = malloc(22*22); // take account of padding
    check_mem(paddedTgt);
    unsigned char *src = malloc(400); 
    check_mem(src);

    int srcVals[] = {
        5,5,1,10,6,7,0,8,10,8,3,8,2,7,2,2,2,5,2,3,8,4,4,2,1,2,3,5,4,6,1,1,9,3,9,8,5,9,7,3,5,4,9,7,9,4,5,5,9,6,5,8,6,1,0,5,1,8,10,10,1,8,8,10,8,1,9,4,5,1,3,6,4,9,6,6,8,3,7,9,5,0,0,0,0,1,5,5,1,6,8,7,10,5,4,4,5,5,10,3,7,2,9,1,0,8,9,6,5,6,7,1,8,1,9,4,10,4,8,10,10,1,9,9,10,10,3,1,7,9,6,8,1,7,0,10,10,10,1,2,0,2,4,9,6,2,9,3,8,10,5,8,5,9,6,6,7,7,1,8,0,2,4,5,7,2,10,0,10,9,10,8,3,10,4,3,3,9,1,7,10,9,8,4,2,5,3,6,6,0,2,8,9,10,7,6,6,8,6,4,3,7,2,4,0,0,0,5,3,6,4,4,10,7,1,4,0,2,2,9,9,4,9,8,1,9,7,4,1,1,6,10,6,7,7,0,7,2,3,3,1,10,10,0,2,9,0,9,4,10,8,6,8,7,1,10,10,9,9,3,3,5,7,10,0,5,6,10,4,3,7,7,2,1,1,0,8,6,9,1,0,1,0,2,0,9,1,5,9,4,0,8,3,4,9,5,3,8,1,10,8,6,1,1,2,5,3,4,5,2,9,6,5,1,4,2,6,3,8,0,4,9,9,1,7,1,2,6,4,4,5,9,9,5,5,7,7,8,7,2,0,8,6,8,4,8,9,2,3,10,8,6,5,1,5,10,9,9,1,4,5,4,5,1,3,8,8,6,10,10,4,0,7,4,7,10,10,8,2,2,5,5,1,1,3,8,0,2,2,4,0,0,7,0,1,1,9,3,8,0
    };

    int expectedVals[] = {  
                            -5,2,-9,7,6,-12,-5,17,-1,2,14,11,-5,-12,10,-16,-3,-3,-12,-2,1,5,13,-21,14,-6,12,-11,-10,-4,-26,-5,8,0,-3,21,15,-20,12,13,-7,-9,8,18,-17,-11,14,-7,6,3,26,-12,-8,7,5,-25,17,8,11,-9,-5,24,-17,13,-15,13,-15,10,-4,7,-15,21,-19,7,-15,12,-15,-18,0,-5,-7,8,-10,5,1,12,-9,5,-3,-23,7,-1,19,-6,19,-5,3,10,-10,12,10,-23,22,-14,5,-16,3,6,14,13,2,-23,9,-8,-6,0,6,2,-4,0,12,-6,-20,23,-3,8,-1,-1,-20,11,2,-2,4,-6,17,-17,2,8,10,2,-18,29,-3,-11,-8,17,-15,-12,8,17,-24,18,-21,9,5,-2,8,-13,-1,-15,12,-9,-13,-5,3,10,-6,9,17,-25,19,-8,-8,-3,14,-15,1,8,13,5,-18,17,3,15,-4,-9,10,1,-18,15,1,-3,7,-3,7,-5,-9,11,-8,-11,-1,20,-23,-5,-1,9,-8,-7,-16,17,-6,5,8,-2,-20,5,10,-4,0,4,-17,5,28,-20,-2,14,-11,2,6,-18,12,2,-7,14,-1,-11,12,-2,-7,-3,16,-5,-13,23,-12,14,-18,19,-5,8,14,-11,-18,21,-9,28,-15,14,-15,8,-10,-15,4,-15,8,-2,19,-4,4,-13,-10,-12,5,-11,0,-10,-13,21,-2,-6,13,0,14,-26,19,-17,-19,-1,0,27,-14,29,14,3,-12,-3,10,-24,11,6,-1,4,5,7,11,1,4,10,15,-22,0,-19,-14,-15,26,-11,-7,7,-2,-1,-8,-7,-7,15,-15,-18,20,-12,-8,-5,-12,26,-5,18,-6,-12,6,15,7,-10,2,18,-12,-19,17,15,-21,-2,13,-2,22,-25,-7,18,14,0,3,-21,8,7,3,-10,19,12,-26,8,5,12,-8,16,-8,7,3,-23,-17,6,22,-9,9,-16,6,-10,-12,8,4,-9,9,-2,-10,1,-2,-9,11,13,-4,-3,-16,15,-19,15
                         };

    for(int i = 0; i < 400; i++) 
        src[i] = (unsigned char) srcVals[i];

    unsigned char *paddedSrc = padImg(src, 20, 20, 0); 
    check(paddedSrc != NULL, "paddedSrc is null, so padding operation failed"); 
    
    int *expected = expectedVals;

    int rc = laplacianTransform(paddedSrc, paddedTgt, 22, 22); 

    printf("\n\nlapl transf completed\n\n");

    int *tgt = trimImg(paddedTgt, 22, 22);

    printf("\n\n trimming completed \n\n");

    mu_assert(rc==0, "laplacianTransform failed");
    rc = memcmp(tgt, expected, 25);
    mu_assert(rc==0, "performing laplacianTransform on src doesn't produce expected values");
    free(tgt);

    return NULL; 

error: 
    return "failed memory or smth";
}

char *test_var() {
    int vals[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 12, 12, 15, 43, 52, 86, 94, 94, 105, 200, 254};
    double expectedVar = 4847.94;

    /*
    int *X = malloc(20);
    check_mem(X);
    for(int i = 0; i < 20; i++) {
        X[i] = vals[i];
    }*/

    int *X = vals;

    double res = var(X, 20);
//    printf("true: %f, calculated: %f\n", expectedVar, res);
    mu_assert(res == expectedVar, "var() didn't produce expected variance");

    return NULL;
error: 
    return "Test failed, memory error";
}
/*
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
        
        IMAGE *img = malloc(sizeof(IMAGE));
        loadBnp(img, "../../34/tape1/0/0.bmp");
        double expectedVol = 800.022539;

        int paddedImgWidth = img->ih.width+2;
        int paddedImgHeight= img->ih.height+2;
        unsigned char *paddedImg = padImg(img->imageData, img->ih.width, img->ih.height, 0);

        img->imageData = paddedImg;

        unsigned char *laplImg = malloc(paddedImgWidth * paddedImgHeight);
        laplacianTransform(paddedImg, laplImg, paddedImgWidth, paddedImgHeight);

        printf("\n\nlaplSuccess\n\n");

        unsigned char *laplImgTrimmed = trimImg(laplImg, paddedImgWidth, paddedImgHeight);

        printf("\n\ntrimSuccess\n\n");

        double vol = calcVol(laplImgTrimmed, paddedImgWidth, paddedImgHeight);

        printf("\n\ncalcVol\n\n");

        printf("\ntrue: %f, calculated: %f\n", expectedVol, vol);
        mu_assert(vol==expectedVol, "calcVol doesn't produce correct vol of tape1 img 0");

        free(img);
        free(paddedImg);
        return NULL;
    }
*/
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