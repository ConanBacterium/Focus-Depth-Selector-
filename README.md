# Focus-Depth-Selector-

This is a program to construct the sharpest possible image from the raw data from a class of digital microscopes that take scans as shown on the image below. The selected focus upon scanning is set based on the center of the images taken, but if the sample being scanned is not perfectly flat some of the objects will be out of focus. This program selects the sharpest parts of the images and constructs an image that has the highest VOL (Variance of Laplacian).

![image](https://github.com/ConanBacterium/Focus-Depth-Selector-/assets/1878037/755f846a-801d-4ac4-81af-404554a4e259)

--------------------
CURRENT THOUGHTS (1): 

For now I ditched the from scratch solution because I couldn't figure out exactly the kernel used in OpenCV, they say it's the sobel operator but I wasted dozens of hours (literally dozens) so I ragequitted. Maybe I will resume it... 

---
3 parseModes:

* One takes vol of each crop and if it's the current biggest vol, the crop gets saved to a static array of crops that will later on be stitched together into full img. 
* Another copies directly into the final full img, thus skipping the final stitching together of the static array
* The last mode saves the pointers to all the crops and only when the snippet will no longer be looked at, the crop is stitched onto the full img. This way no snippet gets overwritten within the full img, thus saving excess copying. 

---
I see no other optimisations apart from loop unrolling (maybe) and using threads. Maybe it makes sense to modify both .py and .c to use custom kernel, and see if it can be made faster by using SIMD and calculate the variance and laplacian in the same pass, this way there should be fewer data transformations and less writing to memory?? 

----------------
CURRENT THOUGHTS (2): 

I added threads. I have 2 modes: 

1) Start 1 masterthread pr band, and everytime an img is loaded and transformed to laplacian spawn a new slavethread that will calculate half of the img variance while the masterthread calculates the other variance. The slave thread terminates after it calculates, and the master thread waits for its termination. 
```
double calcVarianceOfLaplacian2Threads_pthreadRecreated(IplImage* img) {
    imgBuffer = transformImgBlabla();

    createNewThread_calculateVarForHalfOfImg();

    calculateVarForOtherHalfOfImg();

    waitForChildToFinish();

    return mergeTheTwoVariances();
}
```
2) Start 1 masterthread pr band and 1 slavethread per band. So instead of the masterthreads respawning the slave thread, the slavethreads wait for the master thread to signal it to calculate the variance of the buffer, and the slavethread signals to the masterthread when it has finished calculating the variance. This way the threads are only spawned once, and time should be saved because syscalls are expensive. 
```
semProd capacity 0;
semCons capacity 0; 

slaveLoop() {
    while(die == 0) {
        sem_wait(semProd);

        RunningStat_clear(&args->rs);
        for(int i = args->start; i < args->end; i++) {
            RunningStat_push(&args->rs, args->buffer[i]);
        }

        sem_post(args->semCons); 
    }
}

double calcVarianceOfLaplacian2Threads_masterslave(IplImage* img, struct SlaveThreadArgs *slave, long *idle) 
{
    imgBuffer = transformImgBlabla();
    
    sem_post(semProd); 

    struct RunningStat rs; 
    RunningStat_clear(&rs);

    int start = 0;
    int end = 40-1 * CROP_WIDTH;
    for(int i = start; i < end; i++) {
        RunningStat_push(&rs, laplacianData[i]);
    }

    sem_wait(semCons); 
    
    return mergeVariances;
}
```
I probably have a bug, because the second mode is twice as slow. 

 