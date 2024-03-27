#!/bin/bash

# baseline b4 threads 
time ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode1 1 > logs/baselineb4threads_parsemode1_time.txt
time ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode2 2 > logs/baselineb4threads_parsemode2_time.txt
time ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode3 3 > logs/baselineb4threads_parsemode3_time.txt

# multithread PARVAR 
time ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar0 3 0 > logs/multithread_parvar0.txt
time ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar1 3 1 > logs/multithread_parvar1.txt
time ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar2 3 2 > logs/multithread_parvar2.txt

# multithread PARVOL_CROPSYNC
time ./C_multivar_8bands_parVOL_cropsync ../tape1 0 . mutltithread_parvol_cropsync > logs/multithread_parvol_cropsync.txt

#multithread PARVOL_IMGSYNC 
time ./C_multivar_8bands_parVOL_imgsync ../tape1 0 . mutltithread_parvol_imgsync > logs/multithread_parvol_imgsync.txt
