#!/bin/bash

# baseline b4 threads 
echo "baselineb4threads parsemode1"
time ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode1 1 # > logs/baselineb4threads_parsemode1_time.txt
echo "baselineb4threads parsemode2"
time ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode2 2 # > logs/baselineb4threads_parsemode2_time.txt
echo "baselineb4threads parsemode3"
time ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode3 3 # > logs/baselineb4threads_parsemode3_time.txt

# multithread PARVAR 
echo "multithread parvar 0"
time ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar0 3 0 # > logs/multithread_parvar0.txt
echo "multithread parvar 1"
time ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar1 3 1 # > logs/multithread_parvar1.txt
echo "multithread parvar 2"
time ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar2 3 2 # > logs/multithread_parvar2.txt

# multithread PARVOL_CROPSYNC
echo "parvol cropsync"
time ./C_multivar_8bands_parVOL_cropsync ../tape1 0 mutltithread_parvol_cropsync # > logs/multithread_parvol_cropsync.txt

#multithread PARVOL_IMGSYNC 
echo "parvol imgsync"
time ./C_multivar_8bands_parVOL_imgsync ../tape1 0 . mutltithread_parvol_imgsync # > logs/multithread_parvol_imgsync.txt


# PERF CACHE MISSES 
echo ""
echo "PERF CACHE MISSES"
echo ""

# baseline b4 threads 
echo "baselineb4threads parsemode1"
perf stat -e cache-misses ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode1 1 # > logs/baselineb4threads_parsemode1_time.txt
echo "baselineb4threads parsemode2"
perf stat -e cache-misses ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode2 2 # > logs/baselineb4threads_parsemode2_time.txt
echo "baselineb4threads parsemode3"
perf stat -e cache-misses ./C_baseline_b4_threads ../tape1 0 . baselineb4threads_parsemode3 3 # > logs/baselineb4threads_parsemode3_time.txt

# multithread PARVAR 
echo "multithread parvar 0"
perf stat -e cache-misses ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar0 3 0 # > logs/multithread_parvar0.txt
echo "multithread parvar 1"
perf stat -e cache-misses ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar1 3 1 # > logs/multithread_parvar1.txt
echo "multithread parvar 2"
perf stat -e cache-misses ./C_multivar_8bands_parvar ../tape1 0 . multithread_parvar2 3 2 # > logs/multithread_parvar2.txt

# multithread PARVOL_CROPSYNC
echo "parvol cropsync"
perf stat -e cache-misses ./C_multivar_8bands_parVOL_cropsync ../tape1 0 mutltithread_parvol_cropsync # > logs/multithread_parvol_cropsync.txt

#multithread PARVOL_IMGSYNC 
echo "parvol imgsync"
perf stat -e cache-misses ./C_multivar_8bands_parVOL_imgsync ../tape1 0 . mutltithread_parvol_imgsync # > logs/multithread_parvol_imgsync.txt

