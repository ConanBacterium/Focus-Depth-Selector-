// gcc -Wall -g  -O3 -I$HOME/opencv3_build/install/include C_multivar_8bands_parVOL_imgsync.c -L$HOME/opencv3_build/install/lib -Wl,-rpath=$HOME/opencv3_build/install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_multivar_8bands_parVOL_imgsync 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#pragma GCC diagnostic pop
#include <stdio.h>
#include <time.h>
// #include "dbg.h"
#include <pthread.h>
#include <semaphore.h> 
#include <fcntl.h>
#include "parVar.h"

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define CROP_SIZE (2560*80)
#define N_BANDS 8 
#define N_SNIPPETS 324
#define IMG_DEPTH 8
#define N_CHANNELS 1

IplImage *maxVolSnippetPtrs[N_BANDS][N_SNIPPETS]; // for parseMode 3 
double maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main

// this could be done but instead we will do smth easier first, which is just use a single mutex for the whole middle section 
// pthread_mutex_t maxVolSnippetMuts[N_BANDS][24]; // lowest crop on 138 has idx [bandidx][0], second lowest crop on 139 has [bandidx][0] the lowest [bandidx][1], etc etc.
pthread_mutex_t muts[N_BANDS]; 

/*
    So basically two threads on each band, one from 0 to 150 and another from 150 to 301. The danger zone is the 24 images from 138 to and including 161. 
    138 shares bottom crop with top crop of 162.  
*/


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

double calcVarianceOfLaplacian(IplImage* img) {
    IplImage* laplacian = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);

    cvLaplace(img, laplacian, 1);
    
    // Cast imageData to short* for correct type access
    short* laplacianData = (short*)laplacian->imageData;

    double variance = var(laplacianData, CROP_HEIGHT * CROP_WIDTH);

    cvReleaseImage(&laplacian);

    return variance;
}
int saveCropToFullImg(IplImage* cropped, IplImage* fullimg, int bandidx, int snippetidx) {
    char *cdata = cropped->imageData;
    char *fidata= fullimg->imageData;
    int offset = N_BANDS * CROP_WIDTH * CROP_HEIGHT * snippetidx + bandidx * CROP_WIDTH; 

    for(int y = 0; y < CROP_HEIGHT; y++) {
        void *adr = memcpy(&fidata[offset + y * CROP_WIDTH * N_BANDS], &cdata[y*CROP_WIDTH], CROP_WIDTH);
        if(adr != &fidata[offset + y * CROP_WIDTH * N_BANDS]) {
            printf("memcpy failed in saveCropToFullImg!\n");
            return -1;
        }
    }

    return 0;
}

struct DynfocBandArgs {
    int bandidx; 
    IplImage* fullimg;
    char *inputdir; 
    int firstBandNo;
    int intervalStart; 
    int intervalEnd;
};
void *dynfocBand(void *dfbArgs) 
{
    // TODO have a global flag that is set to 1 if there is a failure, right now a thread can fail and noone will know 
    struct DynfocBandArgs *args = (struct DynfocBandArgs *)dfbArgs;

    int bandidx = args->bandidx;
    IplImage* fullimg = args->fullimg;
    char *inputdir = args->inputdir;
    int firstBandNo = args->firstBandNo;
    int intervalStart = args->intervalStart;
    int intervalEnd = args->intervalEnd;

    int nFocusDepths = N_FOCUSDEPTHS;
    int shift = CROP_HEIGHT; 

    IplImage* img; IplImage* cropped;
    struct timespec start, finish;

    char filename[100]; // Adjust the size as necessaryk
    double vol;
    long idle = 0; // used to keep track of how much time the thread is spent waiting for mutex
    // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
    for(int imgidx = intervalStart; imgidx < intervalEnd; imgidx++) {
        snprintf(filename, sizeof(filename), "%s/%d/%d.bmp", inputdir, bandidx+firstBandNo, imgidx);
        img = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
        if (!img) {
            printf("Could not open or find the image.\n");
            return NULL;
        } 
        if (bandidx == 0 && imgidx == 0) {
            printf("imgdepth: %d, nChannels: %d\n", img->depth, img->nChannels);
        }

        int x = 0; int y = 0; 
        int cropsize_x = img->width;
        int cropsize_y = shift;
        for(int cropidx = 0; cropidx < nFocusDepths; cropidx++) {
            int snippetidx = imgidx + cropidx; // NOTE: zero-based indexing
            CvRect roi = cvRect(x, y, cropsize_x, cropsize_y);
            cvSetImageROI(img, roi); 

            cropped = cvCreateImage(cvSize(roi.width, roi.height), img->depth, img->nChannels); // should probably move outside of loops, but it's on stack so whatever compiler will take care of it? 
            cvCopy(img, cropped, NULL); 
            
            vol = calcVarianceOfLaplacian(cropped);

            if(imgidx >= 138 && imgidx <= 162) {
                clock_gettime(CLOCK_MONOTONIC_RAW, &start);
                pthread_mutex_lock(&muts[bandidx]);
                clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
                idle += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000;
            }

            if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                cvReleaseImage(&maxVolSnippetPtrs[bandidx][snippetidx]); // not sure if it's legal to cvReleaseImage(NULL) so maybe this will fail. 
                maxVolSnippetPtrs[bandidx][snippetidx] = cropped;
                maxVolSnippetVols[bandidx][snippetidx] = vol;
            } 

            // stitch the supersnippets that are out of view forever into the fullimg. It will always be the first snippet of every image, and every snippet of the last img 
            if(cropidx == 0 || (imgidx == N_IMGS-1)) {
                int rc = saveCropToFullImg(maxVolSnippetPtrs[bandidx][snippetidx], fullimg, bandidx, snippetidx);
                if( rc != 0) {
                    goto error;
                } 
                cvReleaseImage(&maxVolSnippetPtrs[bandidx][snippetidx]);
            }

            if(imgidx >= 138 && imgidx <= 162) {
                pthread_mutex_unlock(&muts[bandidx]);
            }

            cvResetImageROI(img); 

            y+=shift;
        }

        cvReleaseImage(&img);
        img = NULL;
    }

    printf("Master thread spent : %lu ms waiting for semaphore\n", idle);

    return NULL;
error: 
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 

    return NULL;
}

int main(int argc, char** argv) 
{
    if (argc != 5) {
        printf("Usage: %s inputdir firstbandNo outputdir jobname \n", argv[0]);
        return 1; // Exit with an error code
    }

    char *inputdir = argv[1];
    int firstBandNo = atoi(argv[2]);
    char *outputdir = argv[3];
    char *jobname = argv[4];

    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));
    memset(maxVolSnippetPtrs, 0, sizeof(maxVolSnippetPtrs));

    IplImage* fullimg = cvCreateImage(cvSize(CROP_WIDTH*N_BANDS, CROP_HEIGHT * N_SNIPPETS), IMG_DEPTH, N_CHANNELS);

    pthread_t masterthreads[N_BANDS*2];
    struct DynfocBandArgs dfbArgs[N_BANDS*2];
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        dfbArgs[bandidx].bandidx = bandidx; 
        dfbArgs[bandidx].fullimg = fullimg; 
        dfbArgs[bandidx].inputdir = inputdir;  
        dfbArgs[bandidx].firstBandNo = firstBandNo;
        dfbArgs[bandidx].intervalStart = 0; 
        dfbArgs[bandidx].intervalEnd = 150;

        pthread_create(&masterthreads[bandidx], NULL, dynfocBand, (void *)&dfbArgs[bandidx]);  

        dfbArgs[N_BANDS + bandidx].bandidx = bandidx; 
        dfbArgs[N_BANDS + bandidx].fullimg = fullimg; 
        dfbArgs[N_BANDS + bandidx].inputdir = inputdir;  
        dfbArgs[N_BANDS + bandidx].firstBandNo = firstBandNo;
        dfbArgs[N_BANDS + bandidx].intervalStart = 150; 
        dfbArgs[N_BANDS + bandidx].intervalEnd = 301;

        pthread_create(&masterthreads[N_BANDS + bandidx], NULL, dynfocBand, (void *)&dfbArgs[N_BANDS + bandidx]);  

    }

    for(int i = 0; i < N_BANDS*2; i++) {
        pthread_join(masterthreads[i], NULL);
    }

    char filename[100];
    snprintf(filename, sizeof(filename), "%s.pgm", jobname);
    if (!cvSaveImage(filename, fullimg, NULL)) {
        printf("Could not save the image.\n");
    }

    cvReleaseImage(&fullimg);

    printf("Done!\n");

    return 0;

error: 
    cvReleaseImage(&fullimg);

    return -1; 
}
