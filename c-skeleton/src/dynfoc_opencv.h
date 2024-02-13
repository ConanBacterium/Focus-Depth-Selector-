#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>

double calcVarianceOfLaplacian(IplImage* img, int width, int height);
IplImage *getIplImage(char *srcPath);