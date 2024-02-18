#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <stdio.h>
#include <time.h>
#include "dbg.h"

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define CROP_SIZE (2560*80)
#define N_BANDS 8 
#define N_SNIPPETS 324
#define IMG_DEPTH 8
#define N_CHANNELS 1


struct Crop {
    int imgidx; // which of 0.bnp, 1.bnp, ..., 300.bnp is it cropped from 
    int cropidx; // which of the 24 crops of the img is it
    int snippetidx; // 300+24 idx? so for each interval of 24 imgs each img has 1 snippet that share the same part of the sample
    int vol; // variance of laplacian
};

// so basically... there should only ever be 300+24 snippets saved in memory, because we only need to save the ones with the highest score. 
char maxVolSnippetImageDatas[N_BANDS][N_SNIPPETS][CROP_WIDTH * CROP_HEIGHT]; // ~66mb*8 = ~528mb. should be one dimensional array to facilitate faster stitching back together
double maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main

double vols[N_BANDS*N_IMGS*24]; // 24 for number of focusdepths

struct Crop cropinfos[N_SNIPPETS]; 

void printLplImageAttributes(IplImage* img) {
    printf("align: %d\n", img->align);
    printf("alphaChannel: %d\n", img->alphaChannel);
    printf("BorderConst: {%d, %d, %d, %d}\n",
           img->BorderConst[0], img->BorderConst[1],
           img->BorderConst[2], img->BorderConst[3]);
    printf("BorderMode: {%d, %d, %d, %d}\n",
           img->BorderMode[0], img->BorderMode[1],
           img->BorderMode[2], img->BorderMode[3]);
    printf("channelSeq: %c%c%c%c\n",
           img->channelSeq[0], img->channelSeq[1],
           img->channelSeq[2], img->channelSeq[3]);
    printf("colorModel: %c%c%c%c\n",
           img->colorModel[0], img->colorModel[1],
           img->colorModel[2], img->colorModel[3]);
    printf("dataOrder: %d\n", img->dataOrder);
    printf("depth: %d\n", img->depth);
    printf("height: %d\n", img->height);
    printf("ID: %d\n", img->ID);
    // printf("imageData: %s\n", img->imageData);
    // printf("imageDataOrigin: %s\n", img->imageDataOrigin);
    printf("imageId: %p\n", img->imageId);
    printf("imageSize: %d\n", img->imageSize);
    printf("maskROI: %p\n", img->maskROI);
    printf("nChannels: %d\n", img->nChannels);
    printf("nSize: %d\n", img->nSize);
    printf("origin: %d\n", img->origin);
    printf("roi: %p\n", img->roi);
    printf("tileInfo: %p\n", img->tileInfo);
    printf("width: %d\n", img->width);
    printf("widthStep: %d\n", img->widthStep);
    printf("\n----\n");
}

void printCropInfo(struct Crop* crop) {
    printf("imgidx: %d\ncropidx: %d\nsnippetidx: %d\nvol: %d\n", crop->imgidx, crop->cropidx, crop->snippetidx, crop->vol);
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
    // double variance = var(laplacian->imageData, CROP_HEIGHT*CROP_WIDTH);

    cvReleaseImage(&laplacian);
    // cvReleaseImage(&laplacian64f);

    return variance;
}

double calcVarianceOfLaplacianCustom(IplImage* img) {
    printf("WARNING, calcVarianceOfLaplacianCustom is not finished and thus faulty! ");
    // IplImage* laplacian = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);

    CvMat* kernel = cvCreateMat(3, 3, CV_32FC1);
    CV_MAT_ELEM(*kernel, float, 0, 0) = 0; CV_MAT_ELEM(*kernel, float, 0, 1) = -1; CV_MAT_ELEM(*kernel, float, 0, 2) = 0;
    CV_MAT_ELEM(*kernel, float, 1, 0) = -1; CV_MAT_ELEM(*kernel, float, 1, 1) = 4; CV_MAT_ELEM(*kernel, float, 1, 2) = -1;
    CV_MAT_ELEM(*kernel, float, 2, 0) = 0; CV_MAT_ELEM(*kernel, float, 2, 1) = -1; CV_MAT_ELEM(*kernel, float, 2, 2) = 0;

    // Output image
    IplImage* output_image = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1); // NOTE HERE that the IPL_DEPTH_8U doesn't allow negative values and will clip to 0
    /*
    // Apply the filter
    cvFilter2D(input_image, output_image, kernel, cvPoint(-1, -1));
    
    // Cast imageData to short* for correct type access
    short* laplacianData = (short*)laplacian->imageData;

    double variance = var(laplacianData, CROP_HEIGHT * CROP_WIDTH);
    // double variance = var(laplacian->imageData, CROP_HEIGHT*CROP_WIDTH);

    cvReleaseImage(&laplacian);
    // cvReleaseImage(&laplacian64f);
    */

    cvReleaseImage(&output_image);

    // return variance;
    return 0.0;
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
    if (argc != 7) {
        printf("Usage: %s inputdir firstbandNo outputdir jobname customKernel directParse \n", argv[0]);
        return 1; // Exit with an error code
    }

    char *inputdir = argv[1];
    int firstBandNo = atoi(argv[2]);
    char *outputdir = argv[3];
    char *jobname = argv[4];
    char useCKernel = atoi(argv[5]);
    char directParse = atoi(argv[6]);

    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));
    int nFocusDepths = N_FOCUSDEPTHS;
    int nImgs = N_IMGS;
    int shift = CROP_HEIGHT; 

    IplImage* img; IplImage* cropped;
    IplImage* fullimg = cvCreateImage(cvSize(CROP_WIDTH*N_BANDS, CROP_HEIGHT * N_SNIPPETS), IMG_DEPTH, N_CHANNELS);

    char filename[100]; // Adjust the size as necessaryk
    int vol_i = 0; 
    double vol;
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        int cropcounter = 0; // used to place the crop structs in crops array
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

                if (useCKernel)
                    vol = calcVarianceOfLaplacianCustom(cropped); 
                else 
                    vol = calcVarianceOfLaplacian(cropped);
                check(vol != 0.0, "vol is 0.0!");
                vols[vol_i++] = vol; 
                
                if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                    // if this crop has higher VOL than previous highest of the same snippet then save
                    // printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                    if(directParse) {
                        int rc = saveCropToFullImg(cropped, fullimg, bandidx, snippetidx);
                        if(rc != 0) {
                            goto error;
                        }
                    } else {
                        saveCropToStaticArray(cropped, bandidx, snippetidx);
                    }
                    maxVolSnippetVols[bandidx][snippetidx] = vol;
                }
                cropinfos[cropcounter].imgidx = imgidx;
                cropinfos[cropcounter].cropidx = cropidx;
                cropinfos[cropcounter].snippetidx = snippetidx;
                cropinfos[cropcounter].vol = vol; 

                cvResetImageROI(img); 

                y+=shift;
            }

            cvReleaseImage(&img);
            img = NULL;
            cvReleaseImage(&cropped); 
            cropped = NULL;
        }
    }
    if(!directParse)
        stitchMaxVolSnippetImageDatas(fullimg); 
    
    snprintf(filename, sizeof(filename), "%s.pgm", jobname);
    if (!cvSaveImage(filename, fullimg, NULL)) {
        printf("Could not save the image.\n");
    }

    cvReleaseImage(&fullimg);

    return 0;

error: 
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 
    cvReleaseImage(&fullimg);

    return -1; 
}
