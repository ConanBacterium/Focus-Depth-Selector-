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

void readIntArray(const char *filename, int *array, int length) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < length; i++) {
        if (fscanf(file, "%d,", &array[i]) != 1) {
            fprintf(stderr, "Error reading integer from file: %s\n", filename);
            exit(1);
        }
    }

    fclose(file);
}

char *test_laplacianTransform(){
    int *paddedTgt = malloc(22*22*sizeof(int)); // take account of padding
    check_mem(paddedTgt);
    unsigned char *src = malloc(400); 
    check_mem(src);

    // srcVals and expectedVals were generated by a python script 
    int srcVals[] = {
        0,1,9,1,7,4,0,8,1,0,10,0,9,3,3,2,5,5,4,6,7,8,9,9,3,2,7,5,2,4,7,3,6,1,2,2,7,2,10,6,3,4,8,1,1,8,2,10,8,7,6,8,9,5,10,8,1,5,10,9,2,3,8,10,3,5,0,4,10,9,5,5,9,6,3,1,2,1,5,3,8,7,10,1,10,4,8,8,9,0,6,4,1,2,4,8,5,3,6,3,8,4,10,2,7,4,1,4,2,4,10,4,10,4,0,5,4,10,4,4,6,3,7,9,8,3,0,4,3,7,9,0,2,0,4,1,3,0,1,3,10,9,7,5,4,8,8,3,4,5,9,0,6,3,0,3,6,8,2,1,9,7,8,9,2,1,2,4,1,5,2,10,1,7,6,1,3,10,4,9,6,6,1,6,6,8,3,10,5,10,9,9,6,4,0,8,5,7,1,6,9,1,6,10,0,5,5,0,10,5,8,6,1,4,8,6,2,5,10,1,7,6,0,6,6,8,9,6,7,2,7,5,5,10,0,6,10,3,1,9,2,7,5,8,8,3,4,7,1,3,7,6,0,9,7,10,5,0,1,4,3,5,3,5,9,8,5,10,8,2,8,2,0,3,10,3,10,9,4,6,2,10,7,10,8,0,7,8,1,10,9,8,0,4,1,7,9,1,7,7,6,9,10,1,2,3,10,2,10,0,6,1,4,5,2,4,5,10,10,9,1,5,1,7,6,1,7,7,6,5,9,0,8,1,10,6,4,6,5,2,10,7,4,9,8,2,5,7,0,4,5,3,1,0,10,10,1,9,5,3,3,5,5,0,1,4,7,5,2,10,1,9,10,6,9,1,1,1,4,5,6,7,4,10,4,8,3,7,4,9,8,1,9,3,5,5,8,1,6,3 
    };

    int expectedVals[] = {  
        8,14,-16,22,-13,-3,19,-18,7,15,-23,22,-18,4,-2,4,-1,-4,9,-2,-10,-11,-2,-22,7,14,-19,7,10,0,-5,9,-2,12,8,11,-18,19,-18,7,4,6,-10,24,11,-22,17,-21,-3,-1,3,-9,-8,6,-22,-18,18,-6,-11,-8,8,9,-1,-27,14,-5,19,12,-10,-14,6,6,-15,-5,9,17,0,11,0,8,-7,-3,-14,28,-25,11,-19,-7,-16,28,-5,0,21,7,-3,-17,-3,10,-9,4,-6,12,-17,19,-4,-1,12,-1,12,3,-17,8,-29,-4,17,-7,7,-29,5,-2,3,14,1,-14,-9,8,16,-6,5,-7,-10,15,8,13,-15,11,-1,22,5,-3,-6,-9,1,9,7,-16,-19,8,-4,5,-20,25,-18,1,16,-4,-7,-14,6,11,-4,4,-8,-15,12,16,8,0,14,-2,25,-28,25,-14,-16,16,10,-18,6,-16,6,-9,22,2,-8,-17,13,-28,11,-16,-7,-5,-9,1,26,-20,0,-7,23,-7,-13,23,-12,-22,27,1,-3,31,-23,10,-5,-1,17,7,-22,0,18,2,-32,22,-4,-9,23,0,-2,-9,-13,-1,-9,14,-6,4,-4,-22,31,2,-24,4,19,-21,11,-10,-2,-8,-6,16,8,-7,21,0,-4,-10,20,-16,1,-19,10,18,5,4,0,3,10,10,-7,-15,9,-12,-18,21,-12,14,5,11,-26,25,-14,-21,7,-3,13,-17,5,-19,-11,26,-5,-12,32,-28,-4,-20,16,-7,19,-11,-13,31,-6,1,-6,-5,-22,25,10,1,-21,27,-31,31,-5,14,-2,-9,12,4,7,-18,-9,-8,18,-2,22,-11,-6,14,-5,-6,-2,-1,-20,21,-26,19,-21,4,2,4,3,11,-19,-4,6,-17,-14,10,3,-11,19,4,-3,3,17,18,-11,-22,20,-23,1,3,12,2,-7,25,12,2,-11,3,11,-24,28,-21,-15,-2,-14,21,7,11,1,-5,-2,-6,10,-22,7,-13,13,-9,6,-5,-13,23,-13,11,2,-1,-17,12,-10,5
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

    unsigned char *paddedSrc = padImgReflective(src, 20, 20); 
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

    /*
        printf("\n\n expected: \n\n");
        for(int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                printf("%d,", expected[i*20+j]);
            }
            printf("\n---\n");
        }
    */
    int rc = laplacianTransform(paddedSrc, paddedTgt, 22, 22); 

    // printf("\n\nlapl transf completed\n\n");
    // printf("\n\nwt\n\n");
    int *tgt = trimImgInt(paddedTgt, 22, 22);
    /*
        printf("\n\n tgt: \n\n");
        for(int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                printf("%d,", tgt[i*20+j]);
            }
            printf("\n---\n");
        }
    */
    // printf("\n\n trimming completed \n\n");

    mu_assert(rc==0, "laplacianTransform failed");
    rc = memcmp(tgt, expected, 400*sizeof(int));
    mu_assert(rc==0, "performing laplacianTransform on src doesn't produce expected values");
    free(tgt);
    free(paddedSrc);
    free(paddedTgt);
    free(src);

    printf("\n\nsuccessfully ran lapl transf correctly on 20x20 input\n\n");

    int cols = 2560;
    int rows = 80;

    int src2_int[2560 * 80];
    int expected2[2560 * 80];

    readIntArray("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/c-skeleton/tests/src2560x80nonlaplacian.txt", src2_int, rows * cols);
    readIntArray("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/c-skeleton/tests/tgt2560x80laplacian.txt", expected2, rows * cols);

    unsigned char *src2 = malloc(cols*rows); 
    check_mem(src2);

    for(int i = 0; i < cols*rows; i++) {
        printf("%d,", src2_int[i]);
        src2[i] = (unsigned char) src2_int[i];
    }
    printf("\n\n");
    /*
        for(int i = 0; i < cols*rows; i++) {
            printf("%d,", expected2[i]);
        }
        printf("\n\n");
    */
    unsigned char *paddedSrc2 = padImgReflective(src2, cols, rows); 
    check(paddedSrc2 != NULL, "paddedSrc is null, so padding operation failed");

    int *paddedTgt2 = malloc((rows+2)*(cols+2)*sizeof(int)); // take account of padding
    check_mem(paddedTgt2);
    
    rc = laplacianTransform(paddedSrc2, paddedTgt2, cols+2, rows+2); 
    int *tgt2 = trimImgInt(paddedTgt2, cols+2, rows+2);
    /*
        for(int i = 0; i < cols*rows; i++) {
            printf("%d,", tgt2[i]);
        }
        printf("\n\n");
    */

    mu_assert(rc==0, "laplcianTransform failed on 2560x80 img");
    rc = memcmp(tgt2, &expected2, rows*cols*sizeof(int));
    mu_assert(rc==0, "performing lapl transf on 2560x80 src doesn't produce expected values");
    free(paddedSrc2);
    free(paddedTgt2);
    free(tgt2);

    printf("\n\nLAPLACIAN TRANSFORM ON 2560x80 IMG WORKS!!\n\n");

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