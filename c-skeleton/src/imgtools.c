#include <stdio.h>
#include "imgtools.h"
#include "dbg.h"

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
// heavily chatgpt assisted 
int laplacianTransform(unsigned char *src, int *target, int width, int height) {
    // use short instead of int?  TODO
    if(width < 5 || height < 5) return 1;
    
    printf("width, height ok lapTransf\n");

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

int destroyImage(IMAGE *img) {
    free(img->imageData);
    free(img); 

    return 0; 
}

int loadBnp(IMAGE *img, char *path) {
    // takes uninitialized img pointer, so can be just a malloced pointer without any data set. 
    // TODO: CHECK NULL TERMINATED PATH
    img->isBottomUp = 1;
    FILE *imgf = fopen(path, "rb");
    check(imgf!=NULL, "error opening file");
    int nBytesRead = fread(&img->fh, sizeof(unsigned char), sizeof(FILEHEADER), imgf);
    check(nBytesRead == sizeof(FILEHEADER), "fread didn't read enough bytes, maybe none");
    
    nBytesRead= fread(&img->ih, sizeof(unsigned char), sizeof(INFOHEADER), imgf);
    check(nBytesRead == sizeof(INFOHEADER), "fread didn't read enough bytes, maybe none");

    int height = img->ih.height; 
    int width = img->ih.width;

    img->imageData = malloc(width * height);
    check_mem(img->imageData);
    
    //rewind(img);
    fseek(imgf, img->fh.imageDataOffset, SEEK_SET);
    nBytesRead = fread(img->imageData, 1, width*height, imgf);
    check(nBytesRead == width*height, "fread didn't read enough bytes, maybe none"); 

    return 0;
error: 
    return 1; 
}

int switchLineOrder(IMAGE *img) {
    int width = img->ih.width;
    int height = img->ih.height; 

    unsigned char *tempRow = malloc(width); // malloc... not so good, costly operation for so little a thing
    check_mem(tempRow);
    
    for (int row = 0; row < height / 2; row++) {
        int swapRow = height - row - 1;
        memcpy(tempRow, &img->imageData[row * width], width);
        memcpy(&img->imageData[row * width], &img->imageData[swapRow * width], width);
        memcpy(&img->imageData[swapRow * width], tempRow, width);
    }

    free(tempRow);

    return 0;
error: 
    return -1;
}

double var(int *X, int length) {
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
    /*
    long long sum = 0; long long sumOfSquares = 0; 
    for(int i = 0; i < length; i++) {
        long val = (long)X[i];
        sum += val;
        sumOfSquares += val * val; 
    }
    double mean = (double)sum / length; 
    return ((double)sumOfSquares / length) - (mean * mean);
    */
}

double calcVol(unsigned char *src, int width, int height){
    // variance of laplacian

    int *tgt = malloc(width*height); 
    check_mem(tgt);

    int rc = laplacianTransform(src, tgt, width, height); 
    check(rc==0, "return code of laplacian not zero");

    double vol = var(tgt, width*height); 

    return vol;

error: 
    return -1;
}

unsigned char *padImg(unsigned char *src, int srcWidth, int srcHeight, unsigned char C) {
    int destHeight = srcHeight+2;
    int destWidth = srcWidth+2;
    int destSize = destHeight * destWidth;
    //unsigned char *dest = malloc(srcWidth*srcHeight + srcWidth*2 + srcHeight*2 + 4); 
    unsigned char *dest = malloc(destSize); 
    check_mem(dest);

    for(int i = 0; i < destWidth; i++) dest[i] = C; 
    for(int h = 0; h < srcHeight; h++) {
        dest[(h+1)*destWidth] = C; // put C in start of row
        for(int i = 1; i < destWidth-1; i++) dest[(h+1)*destWidth + i] = src[h*srcWidth+i-1];
        dest[(h+1)*destWidth+destWidth-1] = C; // put C in end of row
    }
    for(int i = 0; i < destWidth; i++) dest[destSize-destWidth+i] = C; 

    return dest;

error: 
    return NULL;
}

unsigned char *trimImg(unsigned char *src, int srcWidth, int srcHeight) {
    // rm padding, so rm first and last column/row
    int destHeight = srcHeight-2;
    int destWidth = srcWidth-2;
    int destSize = destHeight * destWidth;
    unsigned char *dest = malloc(destSize); 
    printf("\n\n171 malloc\n\n");
    check_mem(dest); 

    int tgtIdx = 0;
    for(int h = 0; h < destHeight; h++) {
        for(int i = 0; i < destWidth; i++) {
           dest[tgtIdx] = src[(h+1)*srcWidth+i+1]; // (h+1) to skip first line, and i+1 to skip first row element
           tgtIdx++;
        }
    }



    return dest;

error: 
    return NULL; 

}