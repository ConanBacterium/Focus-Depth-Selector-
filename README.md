# Focus-Depth-Selector-

This is a program to construct the sharpest possible image from the raw data from a class of digital microscopes that take scans as shown on the image below. The selected focus upon scanning is set based on the center of the images taken, but if the sample being scanned is not perfectly flat some of the objects will be out of focus. This program selects the sharpest parts of the images and constructs an image that has the highest VOL (Variance of Laplacian).

![image](https://github.com/ConanBacterium/Focus-Depth-Selector-/assets/1878037/755f846a-801d-4ac4-81af-404554a4e259)

--------------------
CURRENT THOUGHTS: 

For now I ditched the from scratch solution because I couldn't figure out exactly the kernel used in OpenCV, they say it's the sobel operator but I wasted dozens of hours (literally dozens) so I ragequitted. Maybe I will resume it... 

---
3 parseModes:

* One takes vol of each crop and if it's the current biggest vol, the crop gets saved to a static array of crops that will later on be stitched together into full img. 
* Another copies directly into the final full img, thus skipping the final stitching together of the static array
* The last mode saves the pointers to all the crops and only when the snippet will no longer be looked at, the crop is stitched onto the full img. This way no snippet gets overwritten within the full img, thus saving excess copying. 

---
I see no other optimisations apart from loop unrolling (maybe) and using threads. Maybe it makes sense to modify both .py and .c to use custom kernel, and see if it can be made faster by using SIMD and calculate the variance and laplacian in the same pass, this way there should be fewer data transformations and less writing to memory?? 
