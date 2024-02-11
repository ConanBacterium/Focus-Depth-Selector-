// gcc -g comparison.c -o comparison

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_BANDS 8 
#define N_SNIPPETS 324
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80
#define TOTAL_IMG_SIZE (N_BANDS*N_SNIPPETS*CROP_HEIGHT*CROP_WIDTH)

int maxVolSnippetVolsOpenCV[N_BANDS][N_SNIPPETS];  
int maxVolSnippetVolsFromScratch[N_BANDS][N_SNIPPETS];  

unsigned char imageDatas[N_BANDS][N_SNIPPETS][CROP_WIDTH * CROP_HEIGHT]; // ~66mb*8 = ~528mb. should be one dimensional array to facilitate faster stitching back together

void saveAsPGM(const unsigned char* imageData, int width, int height, const char* filename) 
{
    FILE* file = fopen(filename, "wb"); // Open file for writing in binary mode
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Write PGM header
    // P5 denotes the binary version of the format.
    // Width and height specify the dimensions of the image.
    // 255 is the maximum value for each pixel.
    fprintf(file, "P5\n%d %d\n255\n", width, height);

    // Write image data
    fwrite(imageData, sizeof(unsigned char), width * height, file);

    // Close file
    fclose(file);
}

void stitchMaxVolSnippetImageDatas_newchatgpt(unsigned char *dest) {
    // Assuming N_SNIPPETS, N_BANDS, CROP_WIDTH, CROP_HEIGHT are defined elsewhere
    int currentYOffset = 0; 

    // Iterate over each snippet
    for(int snippetidx = 0; snippetidx < N_SNIPPETS; snippetidx++) {
        for (int y = 0; y < CROP_HEIGHT; y++) {
            for(int x = 0; x < CROP_WIDTH; x++) {
                int destY = currentYOffset + y;
                // xOffset is not needed since each band is stitched vertically, not horizontally
                for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
                    // Calculate the correct position in the destination image for the current band
                    int destIndex = (destY + bandidx * CROP_HEIGHT * N_SNIPPETS) * CROP_WIDTH + x;
                    // Copy the pixel value from the source to the destination
                    dest[destIndex] = imageDatas[bandidx][snippetidx][y * CROP_WIDTH + x];
                }
            }
        }
        // After finishing each snippet, move to the next position in the Y direction
        // Since we're stitching vertically, this simply moves down by CROP_HEIGHT after each snippet
        currentYOffset += CROP_HEIGHT;
    }
}

void stitchMaxVolSnippetImageDatas_new(unsigned char *dest) 
{
    int currentYOffset = 0; 
    for(int snippetidx = 0; snippetidx < N_SNIPPETS; snippetidx++) {
        // go through the snippets adding line by line all the bands
        for(int y = 0; y < CROP_HEIGHT; y++) {
            int destY = currentYOffset+y;  // the line in the destinal full img
            for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
                // memcpy line destY of each band
                memcpy(dest + destY * CROP_WIDTH*8 + bandidx * CROP_WIDTH, &imageDatas[bandidx][snippetidx][y * CROP_WIDTH], CROP_WIDTH);
            }
        }
        currentYOffset += CROP_HEIGHT;
    }
}

void stitchMaxVolSnippetImageDatas(unsigned char *dest) 
{
    int currentYOffset = 0; 

    for(int snippetidx = 0; snippetidx < N_SNIPPETS; snippetidx++) {
        for (int y = 0; y < CROP_HEIGHT; y++) {
            for(int x = 0; x < CROP_WIDTH; x++) {
                int destY = currentYOffset+y;
                int xOffset = 0; 
                for(int bandidx = 0; bandidx < N_BANDS; bandidx++) {
                    ((unsigned char *) (dest + destY * CROP_WIDTH))[x+xOffset] = imageDatas[bandidx][snippetidx][y * CROP_WIDTH + x];
            //      ((unsigned char *) (dest + destY * 80))[x+xOffset] = maxVolSnippetImageDatas[bandidx][snippetidx][y * CROP_WIDTH + x];
                    xOffset += CROP_WIDTH;
                }
            }
        }
        currentYOffset += CROP_HEIGHT;
    }
}

int snippetImageDatasToPGM(char *srcPath, char *destPath) 
{

    FILE *idf = fopen(srcPath, "r");

    if(idf == NULL) {
        printf("error opening file\n");
        return -1;
    }

    int itemsRead = fread(&imageDatas, 1, TOTAL_IMG_SIZE, idf);
    fclose(idf);
    if(itemsRead != TOTAL_IMG_SIZE) {
        printf("didn't read whole file\n");
        return -1;
    }
    
    // imageDatas[N_BANDS][N_SNIPPETS][CROP_WIDTH * CROP_HEIGHT]
    /*
    printf("TESTING IF THE BANDS ARE EQUAL!!! If the numbers are the same then the imageDatas are fucked. They can still be fucked though... \n Actually it looks like they're all the same bands, but different focusdepth???");
    printf("\n\n");
    for(int i_band = 0; i_band < N_BANDS; i_band++) {
        for(int i = 0; i < 1000; i++) {
            if(i % 30 == 0)
                printf("%hhu,", imageDatas[i_band][0][i]);
        }
            printf("\n\n");
    }
    printf("\n\n");
    printf("\n\n");
    */


    unsigned char *dest = malloc(TOTAL_IMG_SIZE);
    if(dest == NULL) {
        printf("malloc failed!");
        return -1; 
    }

    stitchMaxVolSnippetImageDatas_new(dest); 


    int bandsize = N_SNIPPETS * CROP_WIDTH * CROP_HEIGHT;
    saveAsPGM(dest, CROP_WIDTH*8, CROP_HEIGHT*N_SNIPPETS, destPath);
    // for(int i = 0; i < 1000; i++) {
    //     printf("%hhu, ", *(dest+TOTAL_IMG_SIZE-i));
    // }

    free(dest);
    return 0; 
}


int main(int argc, char** argv) 
{
    FILE *OCSVolsF = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolSnippetVols_34tape2OpenCV.bytes", "r");
    FILE *FSVolsF = fopen("/mnt/c/Users/jaro/Documents/A_privat_dev/DynamicFocus/C/outputs_for_comparison/maxVolSnippetVols_34tape2FromScratch.bytes", "r");
    
    if(OCSVolsF == NULL || FSVolsF == NULL)  {
        printf("error opening files\n");
        return -1;
    }
    
    int itemsRead = fread(&maxVolSnippetVolsOpenCV, sizeof(int), N_BANDS*N_SNIPPETS, OCSVolsF);
    if(itemsRead!= N_BANDS*N_SNIPPETS) 
        goto error; 

    itemsRead = fread(&maxVolSnippetVolsFromScratch, sizeof(int), N_BANDS*N_SNIPPETS, FSVolsF);
    if(itemsRead != N_BANDS*N_SNIPPETS)
        goto error; 

    for(int i = 0; i < N_BANDS; i++) {
        for(int j = 0; j < N_SNIPPETS; j++) {
            // printf("OC: %d, FS: %d\n", maxVolSnippetVolsOpenCV[i][j], maxVolSnippetVolsFromScratch[i][j]);
            if(maxVolSnippetVolsFromScratch[i][j] != maxVolSnippetVolsOpenCV[i][j]) {
                // printf("FALSE!!!\n");
            } else {
                printf("Found a snippet that had same VOL in OpenCV solution and From Scratch solution\n");
            }
        }
    }

    fclose(OCSVolsF); 
    fclose(FSVolsF);

    int rc = snippetImageDatasToPGM("maxVolSnippetImageDatas_34tape2OpenCV.bytes", "34tape2OpenCV.pgm");
    rc = snippetImageDatasToPGM("maxVolSnippetImageDatas_34tape2FromScratch.bytes", "34tape2FromScratch.pgm");
    if(rc != 0)
        goto error; 

    return 0;  
error: 
    printf("\nERROR\n");
    return -1; 
}