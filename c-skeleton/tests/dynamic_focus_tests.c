#include "minunit.h"
#include "../src/dynamic_focus.h"
#include "../src/dynfoc_opencv.h"



char *testGenDynFocFromTapeDir() {
    printf("\n\ntestGenDynFocFromTapeDir\n\n");
    int rc = genDynFocFromTapeDir();
    mu_assert(rc == 0, "genDynFocFroMTapeDir failed!");

    return NULL; 
}

char *all_tests() {
    mu_suite_start();

    // mu_run_test(testGenDynFocFromTapeDir);

    return NULL;
}

RUN_TESTS(all_tests);