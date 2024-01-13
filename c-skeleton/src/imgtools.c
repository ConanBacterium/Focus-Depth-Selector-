#include <stdio.h>
#include "imgtools.h"
#include "dbg.h"

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
// heavily chatgpt assisted 
int laplacianTransform(unsigned char *src, unsigned char *target, int width, int height) {
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