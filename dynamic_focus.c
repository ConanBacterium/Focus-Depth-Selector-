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
#define N_BANDS 8 
#define N_SNIPPETS 324


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
    cvLaplace(img, laplacian, 3);

    // IplImage* laplacian64f = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);
    // cvConvertScale(laplacian, laplacian64f, 1.0, 0.0);

    // CvScalar mean, stdDev;
    // cvAvgSdv(laplacian, &mean, &stdDev, NULL);
    // double variance = stdDev.val[0] * stdDev.val[0];

    double variance = var(laplacian->imageData, CROP_HEIGHT*CROP_WIDTH);

    cvReleaseImage(&laplacian);
    // cvReleaseImage(&laplacian64f);

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

int main(int argc, char** argv) {
    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));
    int nFocusDepths = N_FOCUSDEPTHS;
    int nImgs = N_IMGS;

    cvNamedWindow("Display window", CV_WINDOW_AUTOSIZE);
    
    IplImage* img; IplImage* cropped;
    img = cvLoadImage("../tape1/0/0.bmp", CV_LOAD_IMAGE_GRAYSCALE);
    if (!img) {
        printf("Could not open or find the image.\n");
        return -1;
    }
    printLplImageAttributes(img);
    // return 0;
    int shift = img->height / N_FOCUSDEPTHS; 
    printf("shift between imgs (calculated as height / #focus depths): %d px\n", shift);

    char filename[100]; // Adjust the size as necessaryk
    int vol_i = 0; 
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        int cropcounter = 0; // used to place the crop structs in crops array
        // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
        for(int imgidx = 0; imgidx < nImgs; imgidx++) {
            snprintf(filename, sizeof(filename), "../34/tape2/%d/%d.bmp", bandidx+8, imgidx);
            img = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
            if (!img) {
                printf("Could not open or find the image.\n");
                return -1;
            }

            ///// these two prints are to get VOLs to have something to test against in own from scratch library
            // if(imgidx==0 && cropidx==0) {
            //     printf("%s has vol %f\n", filename, calcVarianceOfLaplacian(img));
            // }
            // if(imgidx==0) {
            //     printf("%s crop %d has vol %f\n", filename, cropidx, vol);
            // }

            int x = 0; int y = 0; 
            int cropsize_x = img->width;
            int cropsize_y = shift;
            for(int cropidx = 0; cropidx < nFocusDepths; cropidx++) {
                int snippetidx = imgidx + cropidx; // NOTE: zero-based indexing
                CvRect roi = cvRect(x, y, cropsize_x, cropsize_y);
                cvSetImageROI(img, roi); 

                cropped = cvCreateImage(cvSize(roi.width, roi.height), img->depth, img->nChannels); // should probably move outside of loops, but it's on stack so whatever compiler will take care of it? 
                cvCopy(img, cropped, NULL); 

                if(vol_i == 43741) {
                    // for debug purposes. This is the one where OpenCV and from scratch solution diffes the most (the VOL have a 3k diff)
                    FILE *outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolDiffCropOpenCV.bytes", "w");
                    int tmp = fwrite(cropped->imageData, CROP_HEIGHT*CROP_WIDTH, 1, outfile); 
                    check(fclose(outfile)==0, "couldn't close file"); 
                    check(tmp == 1, "write to file failed"); 
                }

                double vol = calcVarianceOfLaplacian(cropped);
                check(vol != 0.0, "vol is 0.0!");
                vols[vol_i++] = vol; 
                
                if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                    // if this crop has higher VOL than previous highest of the same snippet then save
                    // printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                    saveCropToStaticArray(cropped, bandidx, snippetidx);
                    maxVolSnippetVols[bandidx][snippetidx] = vol;
                }
                cropinfos[cropcounter].imgidx = imgidx;
                cropinfos[cropcounter].cropidx = cropidx;
                cropinfos[cropcounter].snippetidx = snippetidx;
                cropinfos[cropcounter].vol = vol; 

                // printCropInfo(&cropinfos[cropcounter]);
                // printf("\n---\n");
                cvResetImageROI(img); 
                // cvShowImage("Display window", cropped);
                y+=shift;
                // goto endAndClean;
            }
        }
    }

    FILE *outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolSnippetImageDatas_34tape2OpenCV.bytes", "w");
    printf("\n\nExpected size of maxVolSnippetImageDatas_34tape2.bytes is %zu\n\n", sizeof(maxVolSnippetImageDatas));
    int itemsWritten = fwrite(&maxVolSnippetImageDatas, sizeof(maxVolSnippetImageDatas), 1, outfile);
    check(itemsWritten == 1, "write to file failed!");
    int rc = fclose(outfile); 
    check(rc==0, "failed clsoing file"); 
    
    outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolSnippetVols_34tape2OpenCV.bytes", "w");
    printf("\n\nExpected size of maxVolSnippetVols_34tape2.bytes is %zu\n\n", sizeof(maxVolSnippetVols));
    itemsWritten = fwrite(&maxVolSnippetVols, sizeof(maxVolSnippetVols), 1, outfile);  // line 82 !!
    check(itemsWritten == 1, "write to file failed!"); 
    rc = fclose(outfile); 
    check(rc==0, "failed closing file"); 

    outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/vols_34tape2OpenCV.bytes", "w");
    printf("\n\nExpected size of vols_34tape2.bytes is %zu\n\n", sizeof(vols));
    itemsWritten = fwrite(&vols, sizeof(vols), 1, outfile);  // line 82 !!
    check(itemsWritten == 1, "write to file failed!"); 
    rc = fclose(outfile); 
    check(rc==0, "failed closing file"); 

    IplImage* dynamicfocusimg = cvCreateImage(cvSize(CROP_WIDTH*N_BANDS, CROP_HEIGHT * N_SNIPPETS), img->depth, img->nChannels);
    stitchMaxVolSnippetImageDatas(dynamicfocusimg); 
    // cvShowImage("Display window", dynamicfocusimg);
    // cvWaitKey(0);

    const char* outfileName = "output_24tape2.pgm"; // Can be .png, .bmp, etc., depending on the desired format
    if (!cvSaveImage(outfileName, dynamicfocusimg, NULL)) {
        printf("Could not save the image.\n");
    }

    cvWaitKey(0);

    // printLplImageAttributes(img);
endAndClean:
    // Release the image
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 

    // Destroy the window
    cvDestroyWindow("Display window");

    return 0;

error: 
    cvReleaseImage(&img);
    cvReleaseImage(&cropped); 

    // Destroy the window
    cvDestroyWindow("Display window");

    return -1; 
}
