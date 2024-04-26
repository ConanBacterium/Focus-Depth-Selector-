/*
gcc -Wall -g -O3 -I$HOME/opencv3_build/install/include C_baseline_b4_threads.c -L$HOME/opencv3_build/install/lib -Wl,-rpath=$HOME/opencv3_build/install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_baseline_b4_threads

TAKEN AND EDITED FROM COMMIT: 
commit 2c63bab94272496e3b13fdeaba60741c9408e6cb
Author: ConanBacterium <jas.rohde@gmail.com>
Date:   Sat Feb 24 13:43:21 2024 +0100

    save before introducing threads
*/

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

int main(int argc, char** argv) 
{
    if (argc != 6) {
        printf("Usage: %s inputdir firstbandNo outputdir jobname parseMode \n", argv[0]);
        return 1; // Exit with an error code
    }

    char *inputdir = argv[1];
    int firstBandNo = atoi(argv[2]);
    char *outputdir = argv[3];
    char *jobname = argv[4];
    char parseMode = atoi(argv[5]);
    if(parseMode < 0 || parseMode > 3)
        printf("parseMode option should be 0 for saving in static array and then stitching together, 1 for saving directly to fullimg, 2 for not saving and not freeing until all focusdepths of snippetidx has been generated and then the 23 lowest are freed and the best is saved directly to fullimg\n"); 

    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));
    memset(maxVolSnippetPtrs, 0, sizeof(maxVolSnippetPtrs));
    int nFocusDepths = N_FOCUSDEPTHS;
    int nImgs = N_IMGS;
    int shift = CROP_HEIGHT; 

    IplImage* img; IplImage* cropped;
    IplImage* fullimg = cvCreateImage(cvSize(CROP_WIDTH*N_BANDS, CROP_HEIGHT * N_SNIPPETS), IMG_DEPTH, N_CHANNELS);

    char filename[100]; // Adjust the size as necessaryk
    double vol;
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
        for(int imgidx = 0; imgidx < nImgs; imgidx++) {
            snprintf(filename, sizeof(filename), "%s/%d/%d.bmp", inputdir, bandidx+firstBandNo, imgidx);
            img = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
            if (!img) {
                printf("Could not open or find the image.\n");
                return -1;
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

                // 2560 x 80 = 124800 bytes = 125 kb

                vol = calcVarianceOfLaplacian(cropped);  // might overwrite the caches? It's a convolution probably, and probably couldn't convolve in-place, but maybe? black box
                
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
    }
    if(parseMode == 1)
        stitchMaxVolSnippetImageDatas(fullimg); 
    
    snprintf(filename, sizeof(filename), "%s.pgm", jobname);
    if (!cvSaveImage(filename, fullimg, NULL)) {
        printf("Could not save the image.\n");
    }

    cvReleaseImage(&fullimg);

    printf("Done!\n");

    return 0;

error: 
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 
    cvReleaseImage(&fullimg);

    return -1; 
}
