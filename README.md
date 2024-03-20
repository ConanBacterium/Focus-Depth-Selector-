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

I added threads. I have 3 modes: 
1) 1 thread per band which - the bands are completely independent of each other. I timed the operations, and it seems that calculating the variance is about ten times slower than applying laplacian transformation on it, so I decided to try to do parallel variance calculations. 

2) Start 1 masterthread pr band, and everytime an img is loaded and transformed to laplacian spawn a new slavethread that will calculate half of the img variance while the masterthread calculates the other variance. The slave thread terminates after it calculates, and the master thread waits for its termination. 
```
double calcVarianceOfLaplacian2Threads_pthreadRecreated(IplImage* img) {
    imgBuffer = transformImgBlabla();

    createNewThread_calculateVarForHalfOfImg();

    calculateVarForOtherHalfOfImg();

    waitForChildToFinish();

    return mergeTheTwoVariances();
}
```
3) Start 1 masterthread pr band and 1 slavethread per band. So instead of the masterthreads respawning the slave thread, the slavethreads wait for the master thread to signal it to calculate the variance of the buffer, and the slavethread signals to the masterthread when it has finished calculating the variance. This way the threads are only spawned once, and time should be saved because syscalls are expensive. 
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
I probably have a bug, because the third mode is twice as slow as second mode. 

(results on my own pc)

FULL: 
jaro@jaro:/mnt/c/Users/jacro/Documents/Code/Dynamic_Focus/Focus-Depth-Selector-$ time ./dynfoc ../tape1/ 0 . tape11 3 2
imgdepth: 8, nChannels: 1
Master thread spent : 217 ms waiting for semaphore
Master thread spent : 215 ms waiting for semaphore
Master thread spent : 251 ms waiting for semaphore
Master thread spent : 193 ms waiting for semaphore
Master thread spent : 232 ms waiting for semaphore
Master thread spent : 183 ms waiting for semaphore
Master thread spent : 214 ms waiting for semaphore
Master thread spent : 220 ms waiting for semaphore
thread signalled to die!
Slave thread spent : 57392 ms waiting for semaphore and : 152 ms being active
thread signalled to die!
thread signalled to die!
Slave thread spent : 57415 ms waiting for semaphore and : 171 ms being active
Slave thread spent : 57416 ms waiting for semaphore and : 129 ms being active
thread signalled to die!
Slave thread spent : 57353 ms waiting for semaphore and : 191 ms being active
thread signalled to die!
Slave thread spent : 57374 ms waiting for semaphore and : 112 ms being active
thread signalled to die!
Slave thread spent : 57393 ms waiting for semaphore and : 153 ms being active
thread signalled to die!
Slave thread spent : 57400 ms waiting for semaphore and : 170 ms being active
thread signalled to die!
Slave thread spent : 57462 ms waiting for semaphore and : 158 ms being active
Done!

real    1m10.892s
user    3m48.312s
sys     0m12.079s
jaro@jaro:/mnt/c/Users/jacro/Documents/Code/Dynamic_Focus/Focus-Depth-Selector-$ time ./dynfoc ../tape1/ 0 . tape11 3 1
imgdepth: 8, nChannels: 1
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Done!

real    1m12.791s
user    4m9.074s
sys     0m14.717s
jaro@jaro:/mnt/c/Users/jacro/Documents/Code/Dynamic_Focus/Focus-Depth-Selector-$ time ./dynfoc ../tape1/ 0 . tape11 3 0
imgdepth: 8, nChannels: 1
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Master thread spent : 0 ms waiting for semaphore
Done!

real    1m15.543s
user    4m40.586s
sys     0m12.580s

(2)
Here I give more results
parVarMode 2: 
real    1m15.072s
user    4m26.119s
sys     0m13.533s
-
real    1m15.036s
user    4m33.778s
sys     0m16.515s
-
real    1m18.255s
user    4m44.498s
sys     0m17.661s

parVarMode 1: 
real    1m19.059s
user    4m36.894s
sys     0m14.376s
-
real    1m17.764s
user    4m41.742s
sys     0m16.341s
-
real    1m7.297s
user    3m48.573s
sys     0m11.936s

parVarMode 0: 
real    1m17.862s
user    5m0.081s
sys     0m14.763s
-
real    1m19.966s
user    5m13.118s
sys     0m13.380s
-
real    1m14.110s
user    4m15.782s
sys     0m9.731s