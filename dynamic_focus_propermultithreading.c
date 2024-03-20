// gcc -Wall -g  -O3 -I$HOME/opencv3_build/install/include dynamic_focus_propermultithreading.c -L$HOME/opencv3_build/install/lib -Wl,-rpath=$HOME/opencv3_build/install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o dynamic_focus_propermultithreading 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#pragma GCC diagnostic pop
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h> 
#include <fcntl.h>

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define CROP_SIZE (2560*80)
#define N_BANDS 8
#define N_SNIPPETS 324
#define IMG_DEPTH 8
#define N_CHANNELS 1

double maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main
IplImage *maxVolSnippetPtrs[N_BANDS][N_SNIPPETS]; // for 

int saveCropToFullImg(IplImage* cropped, IplImage* fullimg, int bandidx, int snippetidx) {
    // printf("saveCropToFullImg bandidx %d, snippetidx %d\n", bandidx, snippetidx);
    char *cdata = cropped->imageData;
    char *fidata= fullimg->imageData;
    int offset = N_BANDS * CROP_WIDTH * CROP_HEIGHT * snippetidx + bandidx * CROP_WIDTH; 

    for(int y = 0; y < CROP_HEIGHT; y++) {
        // printf("attempting memcpy\n");
        void *adr = memcpy(&fidata[offset + y * CROP_WIDTH * N_BANDS], &cdata[y*CROP_WIDTH], CROP_WIDTH);
        // printf("memcpy succeeded\n");
        if(adr != &fidata[offset + y * CROP_WIDTH * N_BANDS]) {
            printf("memcpy failed in saveCropToFullImg!\n");
            return -1;
        }
    }

    return 0;
}

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

struct CropOperationArgs {
    int bandidx;
    int snippetidx; 
    int imgidx;
    int cropidx;
    IplImage *fullimg;
    IplImage *cropped;
};
void *cropOperation(void *coArgs) 
{
    struct CropOperationArgs *args = (struct CropOperationArgs*)coArgs;

    // printf("cropOpeartion bandidx %d snippetidx %d\n", args->bandidx, args->snippetidx);

    double vol = calcVarianceOfLaplacian(args->cropped);
    
    if(maxVolSnippetVols[args->bandidx][args->snippetidx] < vol) {
        // if this crop has higher VOL than previous highest of the same snippet then save
        cvReleaseImage(&maxVolSnippetPtrs[args->bandidx][args->snippetidx]); // not sure if it's legal to cvReleaseImage(NULL) so maybe this will fail. 
        maxVolSnippetPtrs[args->bandidx][args->snippetidx] = args->cropped;
        
        maxVolSnippetVols[args->bandidx][args->snippetidx] = vol;
    }
    // TODO! Should release crop if it wasn't the highest. try writing else block where u release it

    // stitch the supersnippets that are out of view forever into the fullimg. It will always be the first snippet of every image, and every snippet of the last img 
    if(args->cropidx == 0 || (args->imgidx == N_IMGS-1)) {
        int rc = saveCropToFullImg(maxVolSnippetPtrs[args->bandidx][args->snippetidx], args->fullimg, args->bandidx, args->snippetidx);
        if( rc != 0) {
            printf("yikes, saveCropToFullImg failed but I didn't implement how to handle errors in multithreaded version :)) So this crop will just be noise in the final img \n");
        } 
        cvReleaseImage(&maxVolSnippetPtrs[args->bandidx][args->snippetidx]);
    } 
}

struct SlaveThreadArgs {
    sem_t *semProd;
    sem_t *semCons;
    int die;
    int start;
    int end; 
    struct CropOperationArgs *cropOpsBuffer;
};
void *slaveLoop(void *params) //TODO rename
{
    struct SlaveThreadArgs *args = (struct SlaveThreadArgs*)params; 

    struct timespec timestart, timefinish;
    long idle = 0;
    long active = 0;
    while(args->die == 0) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &timestart);
        sem_wait(args->semProd); // wait for master
        // printf("slave awakened!\n");
        clock_gettime(CLOCK_MONOTONIC_RAW, &timefinish);
        idle += (timefinish.tv_sec - timestart.tv_sec) * 1000 + (timefinish.tv_nsec - timestart.tv_nsec) / 1000000; 
        if(args->die) {
            printf("thread signalled to die!\n");
            break;
        }

        // DO TASK
        for(int i = args->start; i < args->end; i++) {
            cropOperation(&args->cropOpsBuffer[i]);
        }
        // printf("slave sleep!\n");
        sem_post(args->semCons); // signal to master that task is done 

        clock_gettime(CLOCK_MONOTONIC_RAW, &timestart);
        active += (timestart.tv_sec - timefinish.tv_sec) * 1000 + (timestart.tv_nsec - timefinish.tv_nsec) / 1000000; 
    }

    printf("Slave thread spent : %lu ms waiting for semaphore and : %lu ms being active \n", idle, active);
}



struct DynfocBandArgs {
    int bandidx; 
    IplImage* fullimg;
    char *inputdir; 
    int firstBandNo;
    struct SlaveThreadArgs *slave;
};
void *dynfocBand(void *dfbArgs) 
{
    // TODO have a global flag that is set to 1 if there is a failure, right now a thread can fail and noone will know 
    struct DynfocBandArgs *args = (struct DynfocBandArgs *)dfbArgs;

    int bandidx = args->bandidx;
    IplImage* fullimg = args->fullimg;
    char *inputdir = args->inputdir;
    int firstBandNo = args->firstBandNo;
    struct SlaveThreadArgs *slave = args->slave;

    int nFocusDepths = N_FOCUSDEPTHS;
    int nImgs = N_IMGS;
    int shift = CROP_HEIGHT; 

    IplImage* img; IplImage* cropped;

    char filename[100]; // Adjust the size as necessaryk
    struct timespec start, finish;
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
        struct CropOperationArgs cropOps[nFocusDepths]; // holds necessary information for running the inner loop, so calculating VOL and updating the maxvol and pasting to fullimg if necessary.
        for(int cropidx = 0; cropidx < nFocusDepths; cropidx++) {
            // if this badboy is parallelized! That would be good, because none of the crops on the same .bmp are dependent on each other since even if they were all of highest vol they could still all be copied. 
            // and the memory imprint would be smaller than further parallelizing the outer loop, since we would load more .bmps
            int snippetidx = imgidx + cropidx; // NOTE: zero-based indexing
            CvRect roi = cvRect(x, y, cropsize_x, cropsize_y);
            cvSetImageROI(img, roi); 

            cropped = cvCreateImage(cvSize(roi.width, roi.height), img->depth, img->nChannels); // should probably move outside of loops, but it's on stack so whatever compiler will take care of it? 
            cvCopy(img, cropped, NULL); 

            cropOps[cropidx] = (struct CropOperationArgs){
                .bandidx = bandidx,
                .snippetidx = snippetidx,
                .imgidx = imgidx,
                .cropidx = cropidx,
                .fullimg = fullimg,
                .cropped = cropped
            };
            
            cvResetImageROI(img); 

            y+=shift;
        }

        // now need to delegate the cropOps to threads... For now there are just one helper thread for this !! 
        slave->start = 0; 
        slave->end = 10; 
        slave->cropOpsBuffer = cropOps;
        sem_post(slave->semProd); // signal to slave that he may start 

        for(int i = slave->end; i < nFocusDepths; i++) {
            cropOperation(&cropOps[i]);
        }

        cvReleaseImage(&img);
        img = NULL;

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        sem_wait(slave->semCons); // wait for slave
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish);
        idle += (finish.tv_sec - start.tv_sec) * 1000 + (finish.tv_nsec - start.tv_nsec) / 1000000; 
    }

    slave->die = 1; 
    printf("Master thread spent : %lu ms waiting for semaphore\n", idle);

    return NULL;
error: 
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 

    return NULL;
}

int main(int argc, char** argv) 
{
    if (argc != 4) {
        printf("Usage: %s inputdir firstbandNo jobname \n", argv[0]);
        return 1; // Exit with an error code
    }

    char *inputdir = argv[1];
    int firstBandNo = atoi(argv[2]);
    char *jobname = argv[3];

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
        dfbArgs[bandidx].inputdir = inputdir;  
        dfbArgs[bandidx].firstBandNo = firstBandNo;
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
            .cropOpsBuffer = NULL,
        }; 

        pthread_create(&masterthreads[bandidx], NULL, dynfocBand, (void *)&dfbArgs[bandidx]);  
        pthread_create(&slavethreads[bandidx], NULL, slaveLoop, (void *)&slavethreadargs[bandidx]); 
    }

    for(int i = 0; i < N_BANDS; i++) {
        pthread_join(masterthreads[i], NULL);
    }

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
