#include "dynamic_focus.h"
#include "dbg.h"

#define N_IMGS 301
#define N_FOCUSDEPTHS 24
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define CROP_SIZE (2560*80)
#define N_BANDS 8 
#define N_SNIPPETS 324
#define SHIFT 80 // same as crop height, it's the image height / N_FOCUSDEPTH

// so basically... there should only ever be 300+24 snippets saved in memory, because we only need to save the ones with the highest score. 
unsigned char maxVolSnippetImageDatas[N_BANDS][N_SNIPPETS][CROP_WIDTH * CROP_HEIGHT]; // ~66mb*8 = ~528mb. should be one dimensional array to facilitate faster stitching back together
double maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main

double vols[N_BANDS*N_IMGS*24]; // 24 for number of focusdepths

// TODO: this function should take the tapedir as an argument
int genDynFocFromTapeDir() 
{
    memset(maxVolSnippetVols, 0, sizeof(maxVolSnippetVols));

    IMAGE *img = malloc(sizeof(IMAGE));
    check_mem(img);

    unsigned char *crop = malloc(CROP_SIZE);
    check_mem(crop);

    unsigned char *paddedCrop = malloc((CROP_HEIGHT+2)*(CROP_WIDTH+2));
    check_mem(paddedCrop); 

    char filename[100]; // Adjust the size as necessaryk
    int vol_i = 0; 
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
        for(int imgidx = 0; imgidx < N_IMGS; imgidx++) {
            snprintf(filename, sizeof(filename), "/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/34/tape2/%d/%d.bmp", bandidx+8, imgidx);
            int rc = loadBnp(img, filename);
            check(rc==0, "failed to load img"); 
            // printf("\n\n loaded bnp! \n\n");

            rc = switchLineOrder(img);
            check(rc==0, "failed to switch line order");
            // printf("\n\nswitched line order!\n\n");

            int x = 0; int y = 0; 
            for(int cropidx = 0; cropidx < N_FOCUSDEPTHS; cropidx++) {
                int snippetidx = imgidx + cropidx; // NOTE: zero-based indexing

                memcpy(crop, &img->imageData[y*CROP_WIDTH + x], CROP_SIZE); // test?? 
                if(vol_i == 43741) {
                    // for debug purposes. This is the one where OpenCV and from scratch solution diffes the most (the VOL have a 3k diff)
                    FILE *outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolDiffCropFromScratch.bytes", "w");
                    int tmp = fwrite(crop, CROP_SIZE, 1, outfile); 
                    check(fclose(outfile)==0, "couldn't close file"); 
                    check(tmp == 1, "write to file failed"); 
                }
                // paddedCrop = padImg(crop, CROP_WIDTH, CROP_HEIGHT, 0);
                paddedCrop = padImgReflective(crop, CROP_WIDTH, CROP_HEIGHT);
                check(paddedCrop != NULL, "padImg returned NULL");

                double vol = calcVol(paddedCrop, CROP_WIDTH, CROP_HEIGHT);
                check(vol != -1, "calcVol returned error (-1)!");
                check(vol != 0.0, "vol is 0.0!");
                vols[vol_i++] = vol;
                
                if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                    // printf("\n\nsnippet has higher vol, copy to maxVolSnippetImageDatas\n\n");
                    // if this crop has higher VOL than previous highest of the same snippet then save
                    // printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                    memcpy(&maxVolSnippetImageDatas[bandidx][snippetidx][0], crop, CROP_SIZE);
                    // printf("\n\ncopied snippet to maxVolSnippetImageDatas\n\n");
                    maxVolSnippetVols[bandidx][snippetidx] = vol;
                }

                y += SHIFT;
            }
            free(paddedCrop);
            paddedCrop = NULL; 
        }
        free(img->imageData); // need to free the imageData after every imgidx iteration, because otherwise new imageData will be allocated in loadBnp function and memory ( A LOT) will leak. 
        img->imageData = NULL; 
    }

    FILE *outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolSnippetImageDatas_34tape2FromScratch.bytes", "w");
    printf("\n\nExpected size of maxVolSnippetImageDatas_34tape1.bytes is %zu\n\n", sizeof(maxVolSnippetImageDatas));
    int itemsWritten = fwrite(&maxVolSnippetImageDatas, sizeof(maxVolSnippetImageDatas), 1, outfile);
    check(itemsWritten == 1, "write to file failed!");
    int rc = fclose(outfile); 
    check(rc==0, "failed clsoing file"); 
    
    outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolSnippetVols_34tape2FromScratch.bytes", "w");
    printf("\n\nExpected size of maxVolSnippetVols_34tape1.bytes is %zu\n\n", sizeof(maxVolSnippetVols));
    itemsWritten = fwrite(&maxVolSnippetVols, sizeof(maxVolSnippetVols), 1, outfile);  // line 82 !!
    check(itemsWritten == 1, "write to file failed!"); 
    rc = fclose(outfile); 
    check(rc==0, "failed closing file"); 

    outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/vols_34tape2FromScratch.bytes", "w");
    printf("\n\nExpected size of vols_34tape1.bytes is %zu\n\n", sizeof(vols));
    itemsWritten = fwrite(&vols, sizeof(vols), 1, outfile);  // line 82 !!
    check(itemsWritten == 1, "write to file failed!"); 
    rc = fclose(outfile); 
    check(rc==0, "failed closing file"); 

    destroyImage(img);
    img = NULL; 
    free(crop);
    crop = NULL;
    free(paddedCrop);
    paddedCrop = NULL; 

    return 0;
error: 
    destroyImage(img); 
    free(crop); 
    free(paddedCrop); 

    return 1; 
}