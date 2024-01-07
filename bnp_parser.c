#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
//#include "dbg.h"

// https://stackoverflow.com/questions/17967480/parse-read-bitmap-file-in-c :-) (http://www.dragonwins.com/domains/getteched/bmp/bmpfileformat.htm)
typedef struct __attribute__((__packed__)) {                                                                                                                                                                                                                             
    unsigned char fileMarker1;                                                                                                                                                                                              
    unsigned char fileMarker2;                                                                                                                                                                                               
    unsigned int   bfSize;                                                                                                                                                                                                                   
    uint16_t unused1;                                                                                                                                                                                                                        
    uint16_t unused2;                                                                                                                                                                                                                        
    unsigned int   imageDataOffset;                                                                                                                                                            
} FILEHEADER;                                                                                                                                                                                                                                

typedef struct __attribute__((__packed__)) {                                                                                                                                                                                                                             
    unsigned int   biSize;                                                                                                                                                                                                                   
    int            width;                                                                                                                                                                
    int            height;                                                                                                                                                                     
    uint16_t planes;                                                                                                                                                                                                                         
    uint16_t bitPix;                                                                                                                                                                                                                         
    unsigned int   biCompression;                                                                                                                                                                                                            
    unsigned int   biSizeImage; // note that this isn't width*height                                                                                                                                                                                                              
    int            biXPelsPerMeter;                                                                                                                                                                                                          
    int            biYPelsPerMeter;                                                                                                                                                                                                          
    unsigned int   biClrUsed;                                                                                                                                                                                                                
    unsigned int   biClrImportant;                                                                                                                                                                                                           
} INFOHEADER;

typedef struct __attribute__((__packed__)) {
    FILEHEADER fh;
    INFOHEADER ih;
    unsigned char *imageData;
} IMAGE;

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
    fclose(imgf);

    return 0;
    
error: 
    fclose(imgf);
    return 1;
}

int clamp(int value, int min, int max); 
// heavily chatgpt assisted 
int laplacianTransform(unsigned char* src, unsigned char* target, int width, int height) {
    // Example 3x3 Laplacian kernel
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

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int main(void) {
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