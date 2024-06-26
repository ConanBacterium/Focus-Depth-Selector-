// gcc -Wall -g  -O3 -I$HOME/opencv3_build/install/include C_multivar_8bands_parvar.c -L$HOME/opencv3_build/install/lib -Wl,-rpath=$HOME/opencv3_build/install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_multivar_8bands_parvar
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

// so basically... there should only ever be 300+24 snippets saved in memory, because we only need to save the ones with the highest score. 
char maxVolSnippetImageDatas[N_BANDS][N_SNIPPETS][CROP_WIDTH * CROP_HEIGHT]; // ~66mb*8 = ~528mb. should be one dimensional array to facilitate faster stitching back together
double maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main

IplImage *maxVolSnippetPtrs[N_BANDS][N_SNIPPETS]; // for parseMode 3 

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

struct pushIntervalToRunningStatArgs {
    struct RunningStat *rs;
    short *buffer;
    int start;
    int end;
};
void *pushIntervalToRunningStat(void *params) 
{
    // TODO: be mindful of buffer overflows?? also probably not right to call it a buffer
    struct pushIntervalToRunningStatArgs *args = (struct pushIntervalToRunningStatArgs*)params;

    struct RunningStat *rs = args->rs; 
    short *buffer = args->buffer;
    int start = args->start;
    int end = args->end;
    for(int i = start; i < end; i++) {
        RunningStat_push(rs, buffer[i]);
    }
}
double calcVarianceOfLaplacian2Threads_pthreadRecreated(IplImage* img, long *idle) 
{
    IplImage* laplacian = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);

    cvLaplace(img, laplacian, 1);
    // TODO ! check if somehow it's possible to do this at the same time as calculating variance - need to know where in the output image it is currently working. Probably not possible unless we change the function

    // Cast imageData to short* for correct type access
    short* laplacianData = (short*)laplacian->imageData;

    struct RunningStat rs1; 
    RunningStat_clear(&rs1);
    struct RunningStat rs2; 
    RunningStat_clear(&rs2);
    // spawn pthread and give it half of the array, then calculate the other half, then join and merge the RunningStats

    pthread_t child;
    struct pushIntervalToRunningStatArgs args1; 
    args1.rs = &rs1;
    args1.buffer = laplacianData;
    args1.start = CROP_WIDTH * 40; 
    args1.end = CROP_WIDTH * 80; 
    pthread_create(&child, NULL, pushIntervalToRunningStat, (void *)&args1);

    struct pushIntervalToRunningStatArgs args2; 
    args2.rs = &rs2;
    args2.buffer = laplacianData;
    args2.start = 0;
    args2.end = CROP_WIDTH * 40-1; 
    pushIntervalToRunningStat((void *)&args2);

    struct timespec tStart, tFinish;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tStart);
    pthread_join(child, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &tFinish);
    *idle += (tFinish.tv_sec - tStart.tv_sec) * 1000 + (tFinish.tv_nsec - tStart.tv_nsec) / 1000000;


    RunningStat_merge(&rs1, &rs2); 

    double variance = RunningStat_variance(&rs1);

    cvReleaseImage(&laplacian);

    return variance;
}
struct SlaveThreadArgs {
    sem_t *semProd;
    sem_t *semCons;
    int die;
    int start;
    int end; 
    short *buffer;
    struct RunningStat rs;
};
void *slaveVariance(void *params) //TODO rename
{
    struct SlaveThreadArgs *args = (struct SlaveThreadArgs*)params; 

    struct timespec start, finish;
    long idle = 0;
    long active = 0;
    while(args->die == 0) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        sem_wait(args->semProd); // wait for master to update RunningStat, start, end and buffer
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
        idle += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000; 
        if(args->die) {
            printf("thread signalled to die!\n");
            break;
        }

        RunningStat_clear(&args->rs);
        for(int i = args->start; i < args->end; i++) {
            RunningStat_push(&args->rs, args->buffer[i]);
        }

        sem_post(args->semCons); // signal to master that task is done 

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        active += (start.tv_sec - finish.tv_sec) * 1000 + (start.tv_nsec - finish.tv_nsec) / 1000000; 

    }
    printf("Slave thread spent : %lu ms waiting for semaphore and : %lu ms being active \n", idle, active);
}
double calcVarianceOfLaplacian2Threads_masterslave(IplImage* img, struct SlaveThreadArgs *slave, long *idle) 
{
    IplImage* laplacian = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);

    cvLaplace(img, laplacian, 1); // TODO change this back from 3 to 1
    // TODO ! check if somehow it's possible to do this at the same time as calculating variance - need to know where in the output image it is currently working. Probably not possible unless we change the function

    // Cast imageData to short* for correct type access
    short* laplacianData = (short*)laplacian->imageData;

    slave->buffer = laplacianData;
    slave->start = 40 * CROP_WIDTH;
    slave->end = 80 * CROP_WIDTH;
    
    sem_post(slave->semProd); // signal to t2 that it can start the variance calculations

    struct RunningStat rs; 
    RunningStat_clear(&rs);
    int start = 0;
    int end = 40-1 * CROP_WIDTH;

    for(int i = start; i < end; i++) {
        RunningStat_push(&rs, laplacianData[i]);
    }

    struct timespec tStart, tFinish;

    clock_gettime(CLOCK_MONOTONIC_RAW, &tStart);
    sem_wait(slave->semCons); // wait for runningstat to be ready 
    clock_gettime(CLOCK_MONOTONIC_RAW, &tFinish);
    *idle += (tFinish.tv_sec - tStart.tv_sec) * 1000 + (tFinish.tv_nsec - tStart.tv_nsec) / 1000000; 

    RunningStat_merge(&rs, &slave->rs); 

    double variance = RunningStat_variance(&rs);

    cvReleaseImage(&laplacian);

    return variance;
}

void saveCropToStaticArray(IplImage* cropped, int bandidx, int maxVolSnippetImageDatasIdx) {
    if (cropped->width != CROP_WIDTH || cropped->height != CROP_HEIGHT) {
        printf("Cropped image size does not match the static array size.\n");
        return;
    }

    for (int y = 0; y < cropped->height; y++) {
        for (int x = 0; x < cropped->width; x++) {
            unsigned char pixelValue = ((unsigned char *)(cropped->imageData + y * cropped->widthStep))[x];
            maxVolSnippetImageDatas[bandidx][maxVolSnippetImageDatasIdx][y * CROP_WIDTH + x] = pixelValue;
        }
    }
}

void stitchMaxVolSnippetImageDatas(IplImage* dest) {
    // TODO here assert that dest is same width as snippets and height is (300+24)*80
    int currentYOffset = 0; 

    for(int snippetidx = 0; snippetidx < N_SNIPPETS; snippetidx++) {
        for (int y = 0; y < CROP_HEIGHT; y++) {
            for(int x = 0; x < CROP_WIDTH; x++) {
                int destY = currentYOffset+y;
                int xOffset = 0; 
                for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
                    ((unsigned char *) (dest->imageData + destY * dest->widthStep))[x+xOffset] = maxVolSnippetImageDatas[bandidx][snippetidx][y * CROP_WIDTH + x];
                    xOffset += CROP_WIDTH;
                }
            }
        }
        currentYOffset += CROP_HEIGHT;
    }
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
    int parseMode; 
    char *inputdir; 
    int firstBandNo;
    int parVarMode;
    struct SlaveThreadArgs *slave;
};
void *dynfocBand(void *dfbArgs) 
{
    // TODO have a global flag that is set to 1 if there is a failure, right now a thread can fail and noone will know 
    struct DynfocBandArgs *args = (struct DynfocBandArgs *)dfbArgs;

    int bandidx = args->bandidx;
    IplImage* fullimg = args->fullimg;
    int parseMode = args->parseMode;
    char *inputdir = args->inputdir;
    int firstBandNo = args->firstBandNo;
    int parVarMode = args->parVarMode;
    struct SlaveThreadArgs *slave = args->slave;

    // printf("thread start with args: \nbandidx %d\nfullimg %p\nparseMode %d\ninputdir %s\nfirstBandNo %d\n\n", bandidx, fullimg, parseMode, inputdir, firstBandNo);

    int nFocusDepths = N_FOCUSDEPTHS;
    int nImgs = N_IMGS;
    int shift = CROP_HEIGHT; 

    IplImage* img; IplImage* cropped;

    char filename[100]; // Adjust the size as necessaryk
    double vol;
    long idle = 0; // used to keep track of how much time the thread is spent waiting for semaphore
    // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
    for(int imgidx = 0; imgidx < nImgs; imgidx++) {
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
            if(parVarMode == 0) 
                vol = calcVarianceOfLaplacian(cropped);
            else if(parVarMode == 1)
                vol = calcVarianceOfLaplacian2Threads_pthreadRecreated(cropped, &idle);
            else if(parVarMode == 2)
                vol = calcVarianceOfLaplacian2Threads_masterslave(cropped, slave, &idle);
            
            if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                // if this crop has higher VOL than previous highest of the same snippet then save
                // printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                if(parseMode == 1) {
                    saveCropToStaticArray(cropped, bandidx, snippetidx);
                    cvReleaseImage(&cropped);
                } else if (parseMode == 2) {
                    int rc = saveCropToFullImg(cropped, fullimg, bandidx, snippetidx);
                    cvReleaseImage(&cropped);
                    if(rc != 0) {
                        goto error;
                    }
                } else if (parseMode == 3) {
                    cvReleaseImage(&maxVolSnippetPtrs[bandidx][snippetidx]); // not sure if it's legal to cvReleaseImage(NULL) so maybe this will fail. 
                    maxVolSnippetPtrs[bandidx][snippetidx] = cropped;
                }
                maxVolSnippetVols[bandidx][snippetidx] = vol;
            } 

            if(parseMode == 3) {
                // stitch the supersnippets that are out of view forever into the fullimg. It will always be the first snippet of every image, and every snippet of the last img 
                if(cropidx == 0 || (imgidx == N_IMGS-1)) {
                    int rc = saveCropToFullImg(maxVolSnippetPtrs[bandidx][snippetidx], fullimg, bandidx, snippetidx);
                    if( rc != 0) {
                        goto error;
                    } 
                    cvReleaseImage(&maxVolSnippetPtrs[bandidx][snippetidx]);
                }
            }

            cvResetImageROI(img); 

            y+=shift;
        }

        cvReleaseImage(&img);
        img = NULL;
        if(parseMode != 3) {
            cvReleaseImage(&cropped); 
            cropped = NULL;
        }
    }

    if(parVarMode == 1)
        printf("Master thread spent : %lu ms waiting for worker to die (sum of waiting times)\n", idle);
    else if(parVarMode == 2)
        printf("Master thread spent : %lu ms waiting for semaphore\n", idle);

    return NULL;
error: 
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 

    return NULL;
}

int main(int argc, char** argv) 
{
    if (argc != 7) {
        printf("Usage: %s inputdir firstbandNo outputdir jobname parseMode parVarMode\n", argv[0]);
        return 1; // Exit with an error code
    }

    char *inputdir = argv[1];
    int firstBandNo = atoi(argv[2]);
    char *outputdir = argv[3];
    char *jobname = argv[4];
    char parseMode = atoi(argv[5]);
    if(parseMode < 0 || parseMode > 3) {
        printf("parseMode option should be 0 for saving in static array and then stitching together, 1 for saving directly to fullimg, 2 for not saving and not freeing until all focusdepths of snippetidx has been generated and then the 23 lowest are freed and the best is saved directly to fullimg\n"); 
        return -1;
    }
    int parVarMode = atoi(argv[6]);
    if(parVarMode < 0 || parVarMode > 3) {
        printf("parVar must be 0 for non-parallel variance and 1 for twothreaded variance with helper thread created and killed pr img and 2 for twothreaded variance with worker thread that is persistent throughout\n");
        return -1;
    }

    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));
    memset(maxVolSnippetPtrs, 0, sizeof(maxVolSnippetPtrs));

    IplImage* fullimg = cvCreateImage(cvSize(CROP_WIDTH*N_BANDS, CROP_HEIGHT * N_SNIPPETS), IMG_DEPTH, N_CHANNELS);

    pthread_t masterthreads[N_BANDS];
    pthread_t slavethreads[N_BANDS];
    struct SlaveThreadArgs slavethreadargs[N_BANDS];
    char *producerNames[] =  {"semProd1", "semProd2", "semProd3", "semProd4", "semProd5", "semProd6", "semProd7", "semProd8"};
    char *consumerNames[] =  {"semCons1", "semCons2", "semCons3", "semCons4", "semCons5", "semCons6", "semCons7", "semCons8"};
    struct DynfocBandArgs dfbArgs[N_BANDS];
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        dfbArgs[bandidx].bandidx = bandidx; 
        dfbArgs[bandidx].fullimg = fullimg; 
        dfbArgs[bandidx].parseMode = parseMode;
        dfbArgs[bandidx].inputdir = inputdir;  
        dfbArgs[bandidx].firstBandNo = firstBandNo;
        dfbArgs[bandidx].parVarMode = parVarMode;
        dfbArgs[bandidx].slave = &slavethreadargs[bandidx];

        sem_unlink(producerNames[bandidx]);
        sem_t *semProd = sem_open(producerNames[bandidx], O_CREAT, 0660, 0);
        if (semProd == SEM_FAILED) {
            printf("sem_open producer open failed!\n");
            return -1;
        }

        sem_unlink(consumerNames[bandidx]);
        sem_t *semCons = sem_open(consumerNames[bandidx], O_CREAT, 0660, 0);
        if (semCons == SEM_FAILED) {
            printf("sem_open producer open failed!\n");
            return -1;
        }

        // note: important that this gets created before the masterthread is created, as otherwise we will have race condition
        slavethreadargs[bandidx] =  (struct SlaveThreadArgs){
            .semProd = semProd,
            .semCons = semCons,
            .die = 0,
            .start = 0,
            .end = 0,
            .buffer = NULL,
            .rs = {
                .m_n = 0,
                .m_oldM = 0, 
                .m_newM = 0, 
                .m_oldS = 0, 
                .m_newS = 0
            }
        }; 

        pthread_create(&masterthreads[bandidx], NULL, dynfocBand, (void *)&dfbArgs[bandidx]);  
        if(parVarMode == 2) {
            pthread_create(&slavethreads[bandidx], NULL, slaveVariance, (void *)&slavethreadargs[bandidx]); 
        }
    }

    for(int i = 0; i < N_BANDS; i++) {
        pthread_join(masterthreads[i], NULL);
    }

    if(parVarMode == 2) {
        for(int i = 0; i < N_BANDS; i++) {
            slavethreadargs[i].die = 1; 
            sem_post(slavethreadargs[i].semProd);
            sem_post(slavethreadargs[i].semCons);
        }

        for (int i = 0; i < N_BANDS; i++) {
            pthread_join(slavethreads[i], NULL);
            sem_close(slavethreadargs[i].semProd);
            sem_close(slavethreadargs[i].semCons);
        }
    }

    if(parseMode == 1)
        stitchMaxVolSnippetImageDatas(fullimg); 
    
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
