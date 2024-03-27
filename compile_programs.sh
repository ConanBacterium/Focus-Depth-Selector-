#!/bin/bash
# C_baseline_b4_threads
gcc -Wall -g -O3 -I$HOME/opencv-install/include C_baseline_b4_threads.c -L$HOME/opencv-install/lib -Wl,-rpath=$HOME/opencv-install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_baseline_b4_threads

# C_multivar_8bands_parvar
gcc -Wall -g -O3 -I$HOME/opencv-install/include C_multivar_8bands_parvar.c -L$HOME/opencv-install/lib -Wl,-rpath=$HOME/opencv-install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_multivar_8bands_parvar

#C_multivar_8bands_parVOL_cropsync
gcc -Wall -g -O3 -I$HOME/opencv-install/include C_multivar_8bands_parVOL_cropsync.c -L$HOME/opencv-install/lib -Wl,-rpath=$HOME/opencv-install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_multivar_8bands_parVOL_cropsync

# C_multivar_8bands_parVOL_imgsync
gcc -Wall -g -O3 -I$HOME/opencv-install/include C_multivar_8bands_parVOL_imgsync.c -L$HOME/opencv-install/lib -Wl,-rpath=$HOME/opencv-install/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -o C_multivar_8bands_parVOL_imgsync

