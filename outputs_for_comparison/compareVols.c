// gcc -g compareVols.c -o compareVols

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_BANDS 8 
#define N_SNIPPETS 324
#define N_IMGS 301
#define VOLS_SIZE (N_BANDS*N_IMGS*24)
#define CROP_WIDTH 2560
#define CROP_HEIGHT 80

double volsOpenCV[VOLS_SIZE];
double volsFromScratch[VOLS_SIZE];

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

int parseVols(char* srcPath, void *dest) 
{
    FILE *f = fopen(srcPath, "r"); 

    if(f == NULL) {
        printf("error opening file!\n");
    }

    int itemsRead = fread(dest, sizeof(double), N_BANDS*N_SNIPPETS*24, f); 
    fclose(f); 
    if(itemsRead != N_BANDS*N_SNIPPETS*24) {
        printf("didn't read whole file\n"); 
        return -1;
    }
}

void findMaxDiffSnippet() 
{
    parseVols("vols_34tape2OpenCV.bytes", &volsOpenCV);
    parseVols("vols_34tape2FromScratch.bytes", &volsFromScratch);

    int maxDiff = 0;  
    int maxDiff_i = 0; 

    double vo, vf, diff;
    int ocSmallers = 0;
    for(int i = 0; i < VOLS_SIZE; i++) {
        vo = volsOpenCV[i];
        vf = volsFromScratch[i];
        // check if OpenCV solution is actually std dev and not variance 
        if(vo*vo == vf) {
            printf("\n\nOpenCV*OpenCV = From Scratch\n\n");
        }
        if(vo > vf) {
            diff = vo-vf;
        }
        else {
            diff = vf-vo;
            ocSmallers++;
        }

        if(diff > maxDiff) {
            maxDiff = diff; 
            maxDiff_i = i;
        }
        // printf("%d: %d | ", i, (int) diff);
        printf("%d: %d | ", i, (int) (vo-vf));
        if(i % 9 == 0) printf("\n");
    }

    printf("\n\nvo %f vf %f\n\n", volsOpenCV[VOLS_SIZE-1], volsFromScratch[VOLS_SIZE-1]);

    printf("\n\nmaxdiff %d idx %d\n", maxDiff, maxDiff_i);
    printf("\n\nocSmallers = %d\n\n", ocSmallers);
}

void compareMaxDiffSnippets() {
    FILE *f = fopen("maxVolDiffCropFromScratch.bytes", "r");
    if(!f) {
        printf("error opening file\n");
        return;
    }
    unsigned char fs[CROP_WIDTH*CROP_HEIGHT];
    int tmp = fread(&fs, 1, sizeof(fs), f);
    fclose(f); 
    if(tmp != CROP_HEIGHT*CROP_WIDTH) {
        printf("didnt read whole file\n"); 
        return; 
    }

    
    f = fopen("maxVolDiffCropOpenCV.bytes", "r");
    if(!f) {
        printf("error opening file\n");
        return;
    }
    unsigned char oc[CROP_WIDTH*CROP_HEIGHT];
    tmp = fread(&oc, 1, sizeof(oc), f);
    fclose(f); 
    if(tmp != CROP_HEIGHT*CROP_WIDTH) {
        printf("didnt read whole file\n"); 
        return; 
    }

    int rc = memcmp(oc, fs, CROP_WIDTH*CROP_HEIGHT);
    if(rc != 0) {
        printf("NOT THE SAME!\n");
    } else {
        printf("COMPLETELY IDENTICAL!\n");
    }


    saveAsPGM(fs, CROP_WIDTH, CROP_HEIGHT, "FSTOPDIFFSNIPPET.pgm"); 
    saveAsPGM(oc, CROP_WIDTH, CROP_HEIGHT, "OCTOPDIFFSNIPPET.pgm"); 
}

int main(int argc, char** argv) 
{
    compareMaxDiffSnippets();

    findMaxDiffSnippet();

    return 0; 
}