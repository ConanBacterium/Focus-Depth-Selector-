#include "minunit.h"
#include "../src/dynfoc_opencv.h"
#include "../src/imgtools.h"


char *test_comparison() {
    IplImage *ocImg = getIplImage("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/c-skeleton/tests/snippet.pgm");
    check(ocImg != NULL, "failed loading image with opencv");

    IMAGE *img = malloc(sizeof(IMAGE));
    check_mem(img);
    int rc = loadBnp(img, "/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/c-skeleton/tests/snippet.bmp");
    mu_assert(rc==0, "failed to load img from scratch solution"); 

    double ocVol = calcVarianceOfLaplacian(ocImg, 2560, 80);

    double fsVol = calcVol(img->imageData, 2560, 80);

    printf("\n\noc: %f & fs: %f\n\n", ocVol, fsVol);

    mu_assert(ocVol == fsVol, "variance of laplacian of opencv and from scratch solutions are not equal");

    destroyImage(img);
    cvReleaseImage(&ocImg);

    return NULL;
error: 
    cvReleaseImage(&ocImg);
    return "Test failed, memory error";
}

char *all_tests() {
    mu_suite_start();

    mu_run_test(test_comparison);

    return NULL;
}

RUN_TESTS(all_tests);