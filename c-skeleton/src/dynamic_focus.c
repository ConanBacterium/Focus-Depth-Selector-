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
int maxVolSnippetVols[N_BANDS][N_SNIPPETS];  // need to set to all 0s in main

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
    for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
        // loop through 301 imgs and get their VOL and the crop with highest vol of a snippet gets saved in maxVolSnippetImageDatas
        for(int imgidx = 0; imgidx < N_IMGS; imgidx++) {
            snprintf(filename, sizeof(filename), "/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/34/tape1/%d/%d.bmp", bandidx, imgidx);
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
                paddedCrop = padImg(crop, CROP_WIDTH, CROP_HEIGHT, 0);
                check(paddedCrop != NULL, "padImg returned NULL");

                double vol = calcVol(paddedCrop, CROP_WIDTH, CROP_HEIGHT);
                check(vol != -1, "calcVol returned error (-1)!");
                check(vol != NULL, "very strange!!!");
                
                if(maxVolSnippetVols[bandidx][snippetidx] < vol) {
                    // printf("\n\nsnippet has higher vol, copy to maxVolSnippetImageDatas\n\n");
                    // if this crop has higher VOL than previous highest of the same snippet then save
                    // printf("VOL %f is new highest of snippet %d\n", vol, snippetidx);
                    memcpy(&maxVolSnippetImageDatas[bandidx][snippetidx][0], crop, CROP_SIZE);
                    // printf("\n\ncopied snippet to maxVolSnippetImageDatas\n\n");
                    maxVolSnippetVols[bandidx][snippetidx] = vol;
                }
                // cropinfos[cropcounter].imgidx = imgidx;
                // cropinfos[cropcounter].cropidx = cropidx;
                // cropinfos[cropcounter].snippetidx = snippetidx;
                // cropinfos[cropcounter].vol = vol; 

                y += SHIFT;
            }
        }
    }

    FILE *outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/maxVolSnippetImageDatas_34tape1.bytes", "w");
    printf("\n\nExpected size of maxVolSnippetImageDatas_34tape1.bytes is %zu\n\n", sizeof(maxVolSnippetImageDatas));
    int itemsWritten = fwrite(&maxVolSnippetImageDatas, sizeof(maxVolSnippetImageDatas), 1, outfile);
    check(itemsWritten == 1, "write to file failed!");
    int rc = fclose(outfile); 
    check(rc==0, "failed clsoing file"); 
    
    outfile = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/maxVolSnippetVols_34tape1.bytes", "w");
    printf("\n\nExpected size of maxVolSnippetVols_34tape1.bytes is %zu\n\n", sizeof(maxVolSnippetVols));
    itemsWritten = fwrite(&maxVolSnippetVols, sizeof(maxVolSnippetVols), 1, outfile); 
    check(itemsWritten == 1, "write to file failed!"); 
    rc = fclose(outfile); 
    check(rc==0, "failed closing file"); 
    free(outfile); 

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