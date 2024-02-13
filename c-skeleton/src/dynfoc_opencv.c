#include "dynfoc_opencv.h"
#include "dbg.h"

double var_short(short *X, int length) {
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

double calcVarianceOfLaplacian(IplImage* img, int width, int height) {
    IplImage* laplacian = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, img->nChannels);
    cvLaplace(img, laplacian, 1);

    short* laplacianData = (short*)laplacian->imageData;
    double variance = var_short(laplacianData, height * width);
    
    cvReleaseImage(&laplacian);

    return variance;
}

IplImage *getIplImage(char *srcPath) {
    IplImage* img;
    
    img = cvLoadImage(srcPath, CV_LOAD_IMAGE_GRAYSCALE);
    if (!img) {
        printf("Could not open or find the image.\n");
        return NULL;
    }
    
    return img; 
}