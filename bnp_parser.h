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

typedef struct __attribute__((__packed__)) {
    unsigned char  b;
    unsigned char  g;
    unsigned char  r;
} RGB;

int parseBnp(IMAGE *img, char *path);

int clamp(int value, int min, int max);

int laplacianTransform(unsigned char *src, unsigned char *target, int width, int height);

double var(unsigned char *X, int length);

double calcVarianceOfLaplacian(unsigned char *imageData, int width, int height);

int dynfoc(unsigned char *dest, char *parentdir, int formatAsBnp);

