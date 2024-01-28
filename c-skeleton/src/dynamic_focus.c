#include <stdio.h>
#include <time.h>
#include "imgtools.h"
#include "dbg.h"

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define CROP_SIZE 2560*80
#define N_BANDS 8 
#define N_SNIPPETS 324
#define SHIFT = 80 // same as crop height, it's the image height / N_FOCUSDEPTH


// so basically... there should only ever be 300+24 snippets saved in memory, because we only need to save the ones with the highest score. 
unsigned char maxVolSnippetImageDatas[N_BANDS][N_SNIPPETS][CROP_WIDTH * CROP_HEIGHT]; // ~66mb*8 = ~528mb. should be one dimensional array to facilitate faster stitching back together
int maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main

// TODO: this function should take the tapedir as an argument
int genDynFocFromTapeDir() {
    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));

    IMAGE *img = malloc(sizeof(IMAGE));
    check_mem(img);

    unsigned char *crop = malloc(CROP_SIZE);
    check_mem(cropped);

    unsigned char *paddedCrop = malloc((CROP_HEIGHT+2)*(CROP_WIDTH+2));
    check_mem(paddedCrop); 

    char filename[100]; // Adjust the size as necessaryk
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        int cropcounter = 0; // used to place the crop structs in crops array
        // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
        for(int imgidx = 0; imgidx < nImgs; imgidx++) {
            snprintf(filename, sizeof(filename), "../34/tape1/%d/%d.bmp", bandidx, imgidx);
            int rc = loadBnp(img, filename);
            mu_assert(rc==0, "failed to load img"); 

            rc = switchLineOrder(img);
            mu_assert(rc==0, "failed to switch line order");

            int x = 0; int y = 0; 
            int cropsize_x = img->width;
            int cropsize_y = shift;
            for(int cropidx = 0; cropidx < nFocusDepths; cropidx++) {
                int snippetidx = imgidx + cropidx; // NOTE: zero-based indexing

                memcpy(crop, img->imageData[y*CROP_WIDTH + x], CROP_SIZE); // test?? 
                paddedCrop = padImg(crop, CROP_WIDTH, CROP_HEIGHT, 0);
                check(paddedCrop != NULL, "padImg returned NULL");
                
                if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                    // if this crop has higher VOL than previous highest of the same snippet then save
                    // printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                    memcpy(&maxVolsnippetImageDatas[bandidx][snippetidx][0], crop, CROP_SIZE);
                    maxVolSnippetVols[bandidx][snippetidx] = vol;
                }
                cropinfos[cropcounter].imgidx = imgidx;
                cropinfos[cropcounter].cropidx = cropidx;
                cropinfos[cropcounter].snippetidx = snippetidx;
                cropinfos[cropcounter].vol = vol; 

                y+=shift;
            }
        }
    }
}