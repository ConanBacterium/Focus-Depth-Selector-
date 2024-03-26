// gcc -Wall -g -O3 -I$HOME/opencv3_build/install/include time_VOL_calculation.c -L$HOME/opencv3_build/install/lib -Wl,-rpath=$HOME/opencv3_build/install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o time_VOL_calculation

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#pragma GCC diagnostic pop
#include <stdio.h>
#include <time.h>

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define CROP_SIZE (2560*80)
#define N_BANDS 8 
#define N_SNIPPETS 324
#define IMG_DEPTH 8
#define N_CHANNELS 1

double var(short *X, int length) {
    //welfords online algorithm
    double mean = 0.0;
    double M2 = 0.0;
    double delta;

    for (int i = 0; i < length; i++) {
        double x = (double)X[i];
        delta = x - mean;
        mean += delta / (i + 1);
        M2 += delta * (x - mean);
    }

    return M2 / length;
}

// long lapltime = 0; 
// long vartime = 0; 
// long bandtime = 0; 
// long bigloadtime = 0; 

double timeCalcVarianceOfLaplacian(IplImage* img, long *createimgtime, long *lapltime, long *vartime) 
{
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    IplImage* laplacian = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
    *createimgtime += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    cvLaplace(img, laplacian, 1);
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
    *lapltime += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000;
    
    // Cast imageData to short* for correct type access
    short* laplacianData = (short*)laplacian->imageData;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    double variance = var(laplacianData, CROP_HEIGHT * CROP_WIDTH);
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
    *vartime += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000;

    cvReleaseImage(&laplacian);

    return variance;
}

int main(int argc, char** argv) 
{
    long createimgtime = 0; // this is actually just for creating an img in calcVarianceOfLaplacian, it's not for loading. lol  
    long lapltime = 0; 
    long vartime = 0; 
    long bigloadtime = 0; // for loading the bmp

    struct timespec start, finish;

    IplImage* cropped;
    IplImage* img;
    char filename[100];
    for (int i = 0; i < 300; i++) {
        snprintf(filename, sizeof(filename), "../tape1/0/%d.bmp", i);

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        img = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
        bigloadtime += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000;
        if (!img) {
            printf("Could not open or find the image.\n");
            return NULL;
        } 

        int x = 0; int y = 0; 
        int cropsize_x = CROP_WIDTH;
        int cropsize_y = CROP_HEIGHT;

        for(int cropidx = 0; cropidx < 24; cropidx++) {
            CvRect roi = cvRect(x, y, cropsize_x, cropsize_y);
            cvSetImageROI(img, roi); 

            cropped = cvCreateImage(cvSize(roi.width, roi.height), IMG_DEPTH, N_CHANNELS); // should probably move outside of loops, but it's on stack so whatever compiler will take care of it? 
            cvCopy(img, cropped, NULL); 
            
            double vol = timeCalcVarianceOfLaplacian(cropped, &createimgtime, &lapltime, &vartime);
        }
    }

    int total = 24*300;
    printf("createimgtime: %ld (total %ld) \n", createimgtime / total, createimgtime);
    printf("lapltime: %ld (total %ld)\n", lapltime / total, lapltime);
    printf("vartime: %ld (total %ld)\n", vartime / total, vartime);
    printf("bigloadtime: %ld (total %ld)\n", bigloadtime / total, bigloadtime);
}



/*
jaro@jaro:/mnt/c/Users/jacro/Documents/Code/Dynamic_Focus/Focus-Depth-Selector-$ ./time_VOL_calculation
createimgtime: 0 (total 0)
lapltime: 2 (total 15120)
vartime: 0 (total 672)
bigloadtime: 2 (total 17939)

--- total = 15120+672+17939 = 33731 
vartime: 672 / 33731 = ~2% of the workload. Makes no sense to optimize it !!!!!!
*/