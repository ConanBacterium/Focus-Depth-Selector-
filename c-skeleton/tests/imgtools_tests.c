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

char *test_padImgReflective() {
    int srcVals[] = {
        1,1,1,1,1,
        1,2,1,1,1,
        1,1,3,1,1,
        1,1,1,4,1,
        1,1,1,1,5
    };
    int tgtVals[] = {
        1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,
        1,1,2,1,1,1,1,
        1,1,1,3,1,1,1,
        1,1,1,1,4,1,1,
        1,1,1,1,1,5,5,
        1,1,1,1,1,5,5
    };

    unsigned char *src = malloc(25);
    check_mem(src);

    for(int i = 0; i < 25; i++) {
        src[i] = (unsigned char) srcVals[i];
    }

    unsigned char *tgt = padImgReflective(src, 5, 5);

    
    printf("\n\n");
    for(int i = 0; i < 49; i++) {
        printf("%d,", tgt[i]);
        if(i%7==6) printf("\n");
    }
    

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
    int *paddedTgt = malloc(22*22*sizeof(int)); // take account of padding
    check_mem(paddedTgt);
    unsigned char *src = malloc(400); 
    check_mem(src);

    // srcVals and expectedVals were generated by a python script 
    int srcVals[] = {
        7,2,9,2,5,10,10,10,7,2,6,1,1,10,6,5,4,10,5,6,5,10,4,4,0,2,8,9,7,5,5,7,8,6,7,3,5,3,0,0,5,4,2,6,6,0,4,8,3,10,9,1,0,9,9,6,4,3,9,5,6,8,9,6,8,9,5,7,6,8,8,8,2,1,10,7,1,4,6,9,4,1,6,10,4,5,2,9,2,10,2,5,5,9,2,2,10,6,5,7,10,3,3,6,0,7,7,9,0,10,9,4,0,3,5,0,2,6,2,9,9,5,3,1,3,6,8,7,9,3,7,1,7,10,8,9,8,7,7,7,6,3,7,3,6,8,4,10,5,1,4,8,1,8,6,8,1,3,2,0,2,10,10,8,9,4,6,5,1,10,10,8,4,2,1,1,0,4,4,6,5,2,7,10,0,5,7,9,6,7,6,1,0,6,5,4,8,3,5,0,7,5,2,9,7,5,9,5,4,2,6,2,7,6,6,9,8,0,7,10,0,7,3,5,10,5,9,5,9,0,10,9,3,7,10,8,0,1,6,7,6,8,10,8,2,3,1,8,7,1,10,3,6,3,5,0,6,1,4,2,1,10,2,5,4,9,4,1,9,5,10,5,6,3,8,3,2,2,9,9,10,10,2,0,8,0,4,6,3,2,0,10,4,3,10,6,1,6,0,1,4,1,7,8,7,1,0,0,9,6,6,10,7,8,8,7,10,7,7,4,4,8,8,6,6,7,10,4,1,3,9,3,5,3,2,1,7,1,5,4,1,10,8,7,7,1,3,0,10,0,3,6,3,1,5,8,10,8,7,9,1,5,3,9,9,10,4,4,0,0,7,2,6,5,9,6,2,6,0,0,10,1,9,7,8,2,0,6,10,8,9,9,4,7,10,1,2,3,3,7
    };

    int expectedVals[] = {  
                           -21,18,-28,10,-8,-23,-12,-14,-9,10,-16,10,15,-27,-2,-7,4,-28,-4,-19,2,-25,9,-4,17,10,-7,-3,-4,4,7,-13,-18,10,-4,11,-6,6,17,11,-5,9,15,-6,-10,21,5,-9,19,-15,-12,20,20,-20,-4,-1,-1,8,-22,-2,-7,-12,-14,9,-7,-18,2,0,-4,2,-5,-16,6,26,-21,-9,21,0,3,-18,1,17,-1,-18,7,2,18,-16,17,-18,24,-1,-4,-25,18,11,-29,1,1,-5,-24,7,6,-10,20,-10,-2,-13,30,-18,-13,-1,19,12,-7,18,16,-7,19,-20,-15,-2,4,11,1,2,-8,8,-21,15,-11,22,-16,-14,-2,-12,-13,-4,-10,-12,-10,16,-9,10,-1,-12,16,-19,1,18,10,-18,23,-13,1,-15,15,2,6,15,13,-23,-8,0,-18,12,-4,6,22,-21,-12,-9,-5,11,10,9,14,-6,1,-20,-9,19,-4,-16,31,-4,1,-13,-3,-4,0,12,18,-11,-3,7,-17,5,-6,21,-18,-2,16,-12,-4,6,-10,7,6,9,-4,15,-17,2,6,-10,-15,19,-7,-26,20,-12,12,10,-21,7,-16,11,-20,22,-15,-18,17,-6,-14,-13,23,3,-5,-10,-15,1,-19,-10,17,5,20,-18,-1,18,-16,18,-9,9,1,22,-21,9,2,12,22,-19,19,-6,8,-25,-1,23,-20,2,-20,9,-6,8,-11,4,4,10,-21,-24,-25,-17,11,23,-21,22,-6,-16,14,6,28,-21,10,13,-15,-3,20,-14,23,9,-1,25,-9,-12,-5,10,15,19,-26,-4,1,-14,-1,-11,-5,-3,-18,-4,-12,-4,-3,-9,-3,5,3,-10,-26,-5,22,4,-21,18,-4,4,9,20,-6,23,-1,2,11,-18,-4,2,-5,23,3,21,-39,16,10,-13,6,12,0,-10,-15,-8,-6,-25,12,-5,19,-10,-2,-24,1,-6,24,15,-14,20,-10,3,-10,-4,16,-11,16,16,-38,20,-25,-2,-14,10,12,-10,-26,-13,-12,-21,6,-9,-23,14,-2,-1,-2,-25 
                         };

    for(int i = 0; i < 400; i++) 
        src[i] = (unsigned char) srcVals[i];
    /*
    printf("\n\n src: \n\n");
    for(int i = 0; i < 20; i++) {
        for (int j = 0; j < 20; j++) {
            printf("%d,", src[i*20+j]);
        }
        printf("\n---\n");
    }
    */

    unsigned char *paddedSrc = padImg(src, 20, 20, 0); 
    check(paddedSrc != NULL, "paddedSrc is null, so padding operation failed"); 

    /*
    printf("\n\n padded src: \n\n");
    for(int i = 0; i < 22; i++) {
        for (int j = 0; j < 22; j++) {
            printf("%d,", paddedSrc[i*22+j]);
        }
        printf("\n---\n");
    }
    */
    
    int *expected = expectedVals;

    // printf("\n\n expected: \n\n");
    // for(int i = 0; i < 20; i++) {
    //     for (int j = 0; j < 20; j++) {
    //         printf("%d,", expected[i*20+j]);
    //     }
    //     printf("\n---\n");
    // }

    int rc = laplacianTransform(paddedSrc, paddedTgt, 22, 22); 

    // printf("\n\nlapl transf completed\n\n");
    // printf("\n\nwt\n\n");
    int *tgt = trimImgInt(paddedTgt, 22, 22);

    // printf("\n\n tgt: \n\n");
    // for(int i = 0; i < 20; i++) {
    //     for (int j = 0; j < 20; j++) {
    //         printf("%d,", tgt[i*20+j]);
    //     }
    //     printf("\n---\n");
    // }

    // printf("\n\n trimming completed \n\n");

    mu_assert(rc==0, "laplacianTransform failed");
    rc = memcmp(tgt, expected, 400);
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
    mu_run_test(test_padImgReflective);
    mu_run_test(test_trimImg);
    mu_run_test(test_laplacianTransform);

    return NULL;
}

RUN_TESTS(all_tests);