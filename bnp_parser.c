#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include "bnp_parser.h"
//#include "dbg.h"

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define N_BANDS 8 
#define N_SNIPPETS 324
#define IMG_HEIGHT 1920

int parseBnp(IMAGE *img, char *path) {
    // TODO: CHECK NULL TERMINATED PATH
    FILE *imgf = fopen(path, "rb");
    if(!imgf) {
        printf("error opening file %s\n", path);
        return 1;
    }
    int nBytesRead = fread(&img->fh, sizeof(unsigned char), sizeof(FILEHEADER), imgf);
    if(nBytesRead != sizeof(FILEHEADER)) {
        printf("fread only read %d bytes instead of the required %ld", nBytesRead, sizeof(INFOHEADER));
        goto error;
    }
    nBytesRead= fread(&img->ih, sizeof(unsigned char), sizeof(INFOHEADER), imgf);
    if(nBytesRead != sizeof(INFOHEADER)) {
        printf("fread only read %d bytes instead of the required %ld", nBytesRead, sizeof(INFOHEADER));
        goto error;
    }
    int height = img->ih.height; int width = img->ih.width;

    img->imageData = malloc(width * height);
    if (!img->imageData) {
        printf("malloc failed for imageData");
        goto error;
    }
    //rewind(img);
    fseek(imgf, img->fh.imageDataOffset, SEEK_SET);
    nBytesRead = fread(img->imageData, 1, width*height, imgf);
    if(nBytesRead != width*height) {
        printf("fread only read %d bytes instead of the required %d", nBytesRead, height*width);
        goto error;
    }

    // BMP are left to right bottom up... So the lowest row is the top row, second lowest row is second highest row etc, so need to swap them. 
    /*
    unsigned char *tempRow = malloc(width); // malloc... not so good, costly operation for so little a thing
    if (!tempRow) {
        printf("malloc failed for tempRow");
        goto error;
    }
    for (int row = 0; row < height / 2; row++) {
        int swapRow = height - row - 1;
       
        memcpy(tempRow, &img->imageData[row * width], width);
        
        // Swap the rows.
        memcpy(&img->imageData[row * width], &img->imageData[swapRow * width], width);
        memcpy(&img->imageData[swapRow * width], tempRow, width);
    }

    free(tempRow);
    */
    fclose(imgf);

    return 0;
    
error: 
    fclose(imgf);
    return 1;
}



int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
// heavily chatgpt assisted 
int laplacianTransform(unsigned char *src, unsigned char *target, int width, int height) {
    int kernel[3][3] = {
        { 0,  1,  0},
        { 1, -4,  1},
        { 0,  1,  0}
    };

    // Loop over the pixels, skipping the border for simplicity
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;
            // Apply the Laplacian kernel
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    sum += src[(y + ky) * width + (x + kx)] * kernel[ky + 1][kx + 1];
                }
            }

            // Assign the computed value to the target pixel
            // Clamp to sure to handle potential underflow or overflow
            int newValue = clamp(sum, 0, 255);
            target[y * width + x] = newValue;
        }
    }

    return 0; // or return a meaningful result or status code
}

double var(unsigned char *X, int length) {
    long long sum = 0; long long sumOfSquares = 0; 
    for(int i = 0; i < length; i++) {
        long val = (long)X[i];
        sum += val;
        sumOfSquares += val * val; 
    }
    double mean = (double)sum / length; 
    return ((double)sumOfSquares / length) - (mean * mean);
}

double calcVarianceOfLaplacian(unsigned char *imageData, int width, int height) {
    // if you observe unexpected behavior maybe width and height aren't proper. imageData should be of length width+height !! 
    unsigned char *tImageData = malloc(width * height);
    if(!tImageData) {
        printf("error malloc tImageData in calcVarianceOfLaplacian");
    }
    int rc = laplacianTransform(imageData, tImageData, width, height);

    double vol = var(tImageData, width * height);
    free(tImageData); 
    return vol;
}


int dynfoc(unsigned char *dest, char *parentdir, int formatAsBnp) {
    // so basically... there should only ever be 300+24 snippets saved in memory, because we only need to save the ones with the highest score. 
    printf("dynfoc1\n");
    unsigned char *maxVolSnippetImageDatas = malloc(N_BANDS * N_SNIPPETS * CROP_WIDTH * CROP_HEIGHT); // ~66mb*8 = ~528mb. should be one dimensional array to facilitate faster stitching back together
    if(!maxVolSnippetImageDatas) {
        printf("couldn't mallo cmaxVolSnippetImageDatas");
        return 1;
    }
    int maxVolSnippetVols[N_BANDS][N_SNIPPETS];  
    printf("dynfoc2\n");
    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));
    int nFocusDepths = N_FOCUSDEPTHS;
    int nImgs = N_IMGS;
    int shift = IMG_HEIGHT / N_FOCUSDEPTHS; 
    int cropsize_x = CROP_WIDTH;
    int cropsize_y = shift;

    IMAGE img;

    img.imageData = malloc(IMG_HEIGHT * CROP_WIDTH);
    if(!img.imageData) {
        printf("malloc failed for img in dynfoc\n");
        return 1;
    }

    char filename[100]; // Maybe adjust size
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        int cropcounter = 0; // used to place the crop structs in crops array
        // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
        for(int imgidx = 0; imgidx < nImgs; imgidx++) {
            snprintf(filename, sizeof(filename), "../tape1/%d/%d.bmp", bandidx, imgidx);
            int rc = parseBnp(&img, filename);
            if(rc != 0) {
                printf("couldn't parse %s\n", filename);
            }

            int x = 0; 
            int y = 0; 
            for(int cropidx = 0; cropidx < nFocusDepths; cropidx++) {
                int snippetidx = imgidx + cropidx; // NOTE: zero-based indexing

                double vol = calcVarianceOfLaplacian(&img.imageData[y*CROP_WIDTH], cropsize_x, cropsize_y);
                if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                    // if this crop has higher VOL than previous highest of the same snippet then save
                    printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                    int idxOfSnippetInMaxVolSnippetImageDatas = (bandidx * N_SNIPPETS*CROP_WIDTH * CROP_HEIGHT) + (snippetidx * CROP_WIDTH * CROP_HEIGHT); // idx of maxVolSnippetImageDatas[bandidx][snippetidx]
                    memcpy(maxVolSnippetImageDatas + idxOfSnippetInMaxVolSnippetImageDatas, &img.imageData[y*CROP_WIDTH], cropsize_x * cropsize_y);
                    maxVolSnippetVols[bandidx][snippetidx] = vol;
                }
                y+=shift;
            }
        }
    }

    // need to parse imageData down to BMP format...
    if(formatAsBnp) {
        for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
            int yOffset = N_SNIPPETS-1; 
            for(int snippetidx = 0; snippetidx < N_SNIPPETS; snippetidx++) { 
                for(int snippetRow = 0; snippetRow < CROP_HEIGHT; snippetRow++) {
                    int idxTargetRow = yOffset * N_BANDS * CROP_WIDTH;
                    int idxOfSnippetInMaxVolSnippetImageDatas = (bandidx * N_SNIPPETS*CROP_WIDTH * CROP_HEIGHT) + (snippetidx * CROP_WIDTH * CROP_HEIGHT); // idx of maxVolSnippetImageDatas[bandidx][snippetidx]
                    memcpy(dest + idxTargetRow + (bandidx * CROP_WIDTH), maxVolSnippetImageDatas + idxOfSnippetInMaxVolSnippetImageDatas+snippetRow*CROP_WIDTH, cropsize_x * cropsize_y);
                    yOffset -= 1;
                }
            }

        }
    } else {
        for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
            int yOffset = 0; 
            for(int snippetidx = 0; snippetidx < N_SNIPPETS; snippetidx++) {
                for(int snippetRow = 0; snippetRow < CROP_HEIGHT; snippetRow++) {
                    int idxTargetRow = yOffset * N_BANDS * CROP_WIDTH; // beginning of the target row in src img, so always x=0.
                    int beginningOfTargetBand = (bandidx * N_SNIPPETS * CROP_WIDTH * CROP_HEIGHT);
                    int idxOfSnippetInMaxVolSnippetImageDatas = beginningOfTargetBand + (snippetidx * CROP_WIDTH * CROP_HEIGHT);
                    int destX = idxTargetRow + (bandidx * CROP_WIDTH); // distance in x axis 
                    memcpy( dest + idxTargetRow + (destX), 
                            maxVolSnippetImageDatas + idxOfSnippetInMaxVolSnippetImageDatas+snippetRow * CROP_WIDTH, 
                            CROP_WIDTH);
                    yOffset++;
                }
            }
        }

    }

    return 0;
}

int main() {
    printf("main1\n");
    
    unsigned char *finalImageData = malloc(N_BANDS * N_SNIPPETS * CROP_WIDTH * CROP_HEIGHT); 
    if(!finalImageData) {
        printf("couldn't mallo cmaxVolSnippetImageDatas");
        return 1;
    }

    int rc = dynfoc(finalImageData, "this should be parentdir, so the directory where the 8 band directories are stored, but right now it's a literal in the code ../tape1/", 0);
    // int rc = 0;
    if(rc != 0 ) {
        printf("dynfoc failed"); 
        return 1; 
    }

    IplImage* img = cvCreateImageHeader(cvSize(20480, 25920), IPL_DEPTH_8U, 1);
    img->imageData = finalImageData;

    // Display the image
    cvNamedWindow("Display window", CV_WINDOW_AUTOSIZE);
    cvShowImage("Display window", img);
    cvWaitKey(0);

    // Clean up
    cvReleaseImageHeader(&img);

    RGB grayPalette[256];
    for (int i = 0; i < 256; ++i) {
        grayPalette[i].r = grayPalette[i].g = grayPalette[i].b = i;
    }

    FILEHEADER fh;
    INFOHEADER ih;

    // Bitmap file header setup
    fh.fileMarker1 = 'B';
    fh.fileMarker2 = 'M';
    fh.unused1 = 0;
    fh.unused2 = 0;
    fh.imageDataOffset = sizeof(FILEHEADER) + sizeof(INFOHEADER) + sizeof(grayPalette);

    // Bitmap info header setup
    ih.biSize = sizeof(INFOHEADER);
    ih.width = 20480;
    ih.height = 25920;
    ih.planes = 1; // Always 1 for BMP files
    ih.bitPix = 8; // 8 bits per pixel for grayscale
    ih.biCompression = 0; // No compression
    ih.biXPelsPerMeter = 0; // Resolution not specified
    ih.biYPelsPerMeter = 0; // Resolution not specified
    ih.biClrUsed = 256; // For 8-bit grayscale, 256 colors are used (0-255)
    ih.biClrImportant = 0; // All colors are important

    // Calculate the size of the image data (including padding)
    int rowSize = ((ih.width * ih.bitPix + 31) / 32) * 4; // Row size must be a multiple of 4 bytes
    ih.biSizeImage = rowSize * ih.height;

    // Total file size
    fh.bfSize = fh.imageDataOffset + ih.biSizeImage; 
    printf("\nrowSize: %d\n", rowSize);
    printf("imageDataOffset: %d\n", fh.imageDataOffset);
    printf("biSizeImage: %d\n", ih.biSizeImage);
    printf("FILEHEADER: %ld\n", sizeof(FILEHEADER));
    printf("INFOHEADER: %ld\n", sizeof(INFOHEADER));
    printf("bfSize (total file size): %d\n", fh.bfSize);


    FILE* file = fopen("fromscreatch_dynfoc.bmp", "wb");
    if (file == NULL) {
        printf("file couldn't be opened");
    }

    fwrite(&fh, sizeof(FILEHEADER), 1, file);
    fwrite(&ih, sizeof(INFOHEADER), 1, file);
    fwrite(&grayPalette, sizeof(grayPalette), 1, file); 
    fwrite(finalImageData, sizeof(unsigned char), ih.width * ih.height, file);

    fclose(file);
    free(finalImageData);


    return 0;
}



int main_old(void) {
    IMAGE *customImg = malloc(sizeof(IMAGE)); // very costly operation for only 56 bytes or however much this is. 
    if(!customImg) {
        printf("malloc failed for IMAGE struct");
        return 1;
    }
    int rc = parseBnp(customImg, "../tape1/0/0.bmp");
    if(rc != 0) {
        printf("Error parsing bnp, return code %d\n", rc); 
        return 1;
    }
    unsigned char *transfImageData = malloc(customImg->ih.width * customImg->ih.height); 
    if(!transfImageData) {
        printf("malloc failed for transfImageData");
        return 1;
    }
    rc = laplacianTransform(customImg->imageData, transfImageData, customImg->ih.width, customImg->ih.height);
    if(rc != 0) {
        printf("error during laplacian transformation"); 
        return 1; 
    }
    customImg->imageData = transfImageData;

    cvNamedWindow("Display window", CV_WINDOW_AUTOSIZE);
    IplImage* img = cvLoadImage("../tape1/0/0.bmp", CV_LOAD_IMAGE_GRAYSCALE);
    const char* outfileName = "opencv_parsed.png"; // Can be .png, .bmp, etc., depending on the desired format
    if (!cvSaveImage(outfileName, img, NULL)) {
        printf("Could not save the image.\n");
    }

    cvShowImage("Display window", img);
    printf("Displaying bmp opened with openCV, press a key to continue\n");
    cvWaitKey(0);
    printf("Changing the underlying data behind the image, press a key to continue\n");
    img->imageData = customImg->imageData;
    cvWaitKey(0);
    printf("Done! If nothing happened, then press a key to continue\n");
    cvWaitKey(0);
    cvShowImage("Display window", img);
    printf("There. Should display img but laplassian transformed. If nothing happened now it's very odd...!\n"); 
    cvWaitKey(0);

    cvReleaseImage(&img);
    free(customImg); 

    // REMEMBER TO FREE MEMORY !!! 

    return 0;
} 