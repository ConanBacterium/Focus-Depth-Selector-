#include <stdio.h>
#include "imgtools.h"
#include <limits.h>
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
    
    // printf("width, height ok lapTransf\n");

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
            // ACTUALLY DONT DO THIS !!! we need the negatives
            // int newValue = clamp(sum, 0, 255);
            int newValue = sum;

            check(y * width + x <= width*height, "should never happen");
            // printf("\n\nidx %d, newValue %d\n\n", y * width + x, newValue);
            // SEGFAULT AT SECOND CALL OF THIS FUNC IDX "idx 84420, newValue -519"
            // with valgrind it is always at "idx 202238, newValue 17"
            target[y * width + x] = newValue;
        }
    }

    return 0; // or return a meaningful result or status code

error: 
    return -1;
}

int destroyImage(IMAGE *img) {
    free(img->imageData);
    img->imageData = NULL; 
    free(img); 
    img = NULL; 

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

    int rc = fclose(imgf);
    if(rc != 0) {
        return -1;
    }

    return 0;
error: 
    fclose(imgf); 
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
}

double calcVol(unsigned char *src, int width, int height){
    // printf("\n\n CALC VOL !!!! width*height=%d\n\n", width*height);
    // variance of laplacian

    int *tgt = malloc(width * height * sizeof(int));  
    check_mem(tgt);

    int rc = laplacianTransform(src, tgt, width, height); 
    if(rc != 0) {
        free(tgt); 
        printf("\n\nreturn code of laplacian not zero!\n");
        goto error;
    }

    double vol = var(tgt, width*height); 

    free(tgt);
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

unsigned char *padImgReflective(unsigned char *src, int srcWidth, int srcHeight) {
    /*
        Not sure how to reflect the corners, but it doesn't matter with the laplacian since the corners of the kernel are 0 anyway
    */
    // printf("\n\npadImgReflective\n\n");
    int destHeight = srcHeight+2;
    int destWidth = srcWidth+2;
    int destSize = destHeight * destWidth;
    // printf("\n\ndestSize %d\n\n", destSize);
    //unsigned char *dest = malloc(srcWidth*srcHeight + srcWidth*2 + srcHeight*2 + 4); 
    unsigned char *dest = malloc(destSize); 
    check_mem(dest);

    // reflect first row, putting 0 in corners 
    dest[0] = 0; 
    // printf("\n\nTrying to memcpy first line, dest[1], src[0], srcWidth: %d\n\n", srcWidth);
    void *tmp  = memcpy(&dest[1], &src[0], srcWidth); 

    // printf("\n\ntmp %p == %p\n\n", tmp, &dest[1]);

    if(tmp != &dest[1]) {
        printf("\n\nmemcpy failed\n\n");
        free(dest); 
        return NULL; 
    }

    // printf("\n\nDidn't fail memcpy first line\n\n");
    dest[destWidth-1] = 0; 
    // printf("\n\n padded first row\n\n");
    
    // copy src adding reflected borders
    for(int y = 1; y < destHeight-1; y++) {
        // DEBUG: destWidth, destHeight=10, srcWidth, srcHeight=9, y=9
        // printf("\n\n1: dest[%d] = src[%d]\n\n", destWidth*y, srcWidth*(y-1));
        dest[destWidth*y] = src[srcWidth*(y-1)];  // dest[90] = src[80]
        // printf("\n\n2: dest[%d] = src[%d]\n\n", destWidth*y+1, srcWidth*(y-1));
        tmp = memcpy(&dest[destWidth*y + 1], &src[srcWidth*(y-1)], srcWidth); // dest[91] = src[81]...src[89]
        if( tmp != &dest[destWidth*y + 1]) {
            printf("\n\nmemcpy failed\n\n");
            free(dest); 
            return NULL;
        }
        // printf("\n\n3: dest[%d] = src[%d]\n\n", destWidth*(y + 1), srcWidth*(y+1)-1);
        dest[destWidth*(y + 1) - 1] = src[srcWidth*(y) - 1]; // dest[99] = src[89]
    }

    // reflect last row, putting 0 in corners 
    // printf("\n\n4: dest[%d] = 0\n\n", destWidth*(destHeight-1));
    dest[destWidth*(destHeight-1)] = 0; // dest[90]
    // printf("\n\n5: dest[%d] = src[%d]\n\n", destWidth*(destHeight-1)+1, srcWidth*(srcHeight-1));
    tmp = memcpy(&dest[destWidth*(destHeight-1)+1], &src[srcWidth*(srcHeight-1)], srcWidth); // dest[91] = src[81]...src[89]
    if( tmp != &dest[destWidth*(destHeight-1)+1]) {
        printf("\n\nmemcpy failed\n\n");
        free(dest); 
        return NULL;
    }
    // printf("\n\n6: dest[%d] = 0\n\n", destWidth*(destHeight)-1);
    dest[destWidth*(destHeight)-1] = 0; // dest[99]

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

int *trimImgInt(int *src, int srcWidth, int srcHeight) {
    // rm padding, so rm first and last column/row
    int destHeight = srcHeight-2;
    int destWidth = srcWidth-2;
    int destSize = destHeight * destWidth * sizeof(int);
    int *dest = malloc(destSize); 
    check_mem(dest); 

    int tgtIdx = 0;
    for(int h = 0; h < destHeight; h++) {
        for(int i = 0; i < destWidth; i++) {
            int srcIdx =  (h+1)*srcWidth+i+1; // (h+1) to skip first line, and i+1 to skip first row element
            // printf("\n\n tgt idx %d - src idx %d \n\n", tgtIdx, srcIdx);
            dest[tgtIdx] = src[srcIdx];  
            tgtIdx++;
        }
    }

    return dest;

error: 
    return NULL; 
}
