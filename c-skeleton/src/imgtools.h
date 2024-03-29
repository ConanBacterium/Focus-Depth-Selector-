#include <stdint.h>
#include <stdlib.h>


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
    int isBottomUp;
    unsigned char *imageData;
} IMAGE;

typedef struct __attribute__((__packed__)) {
    unsigned char  b;
    unsigned char  g;
    unsigned char  r;
} RGB;

int destroyImage(IMAGE *img); 
int loadBnp(IMAGE *img, char *path);
int switchLineOrder(IMAGE *img);

int laplacianTransform(unsigned char *src, int *target, int width, int height);
int laplacianTransform_new(unsigned char *src, int *target, int width, int height);

double var(int *X, int length);

double calcVol(unsigned char *src, int width, int height);

unsigned char *padImg(unsigned char *src, int srcWidth, int srcHeight, unsigned char C);
unsigned char *padImgReflective(unsigned char *src, int srcWidth, int srcHeight);

unsigned char *trimImg(unsigned char *src, int srcWidth, int srcHeight);
int *trimImgInt(int *src, int srcWidth, int srcHeight);