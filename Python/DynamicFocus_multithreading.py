import cv2
import numpy as np
import math
from PIL import Image
from PIL.PngImagePlugin import PngInfo
import PIL 
from pathlib import Path
import time
import shutil
import os
import sys 
from collections import defaultdict
import threading


###### the following is a test to see if the output matches the one from the C OpenCV solution. 
# try: 
#     img = Image.open("snippet_125.549453.bmp")
# except PIL.UnidentifiedImageError as PUI:
#     print(PUI)

# gray_img= cv2.cvtColor(np.array(img), cv2.COLOR_BGR2GRAY)
# gray_img_np = np.array(gray_img)
# fm = cv2.Laplacian(gray_img_np, cv2.CV_64F, ksize=1, borderType=cv2.BORDER_REFLECT).var() # blur score 
# print(fm)

N_IMGS = 301 
N_FOCUSDEPTHS = 24
CROP_WIDTH = 2560
CROP_HEIGHT = 80 
N_BANDS = 8
N_SNIPPETS = 324

maxVolSnippetImgs = [0 for i in range(N_BANDS*N_SNIPPETS)]
maxVolSnippetVols = [0 for i in range(N_BANDS*N_SNIPPETS)]

if len(sys.argv) < 3:
    print("Usage: python DynamicFocus.py inputdir outputdir jobname")
    sys.exit(1)

input_dir = Path(sys.argv[1])
output_dir = Path(sys.argv[2])
job_name = sys.argv[3]

bands_dirs = [dir for dir in input_dir.iterdir() if dir.is_dir()]
bands_dirs = [(b, int(b.stem)) for b in bands_dirs ]
bands_dirs = sorted(bands_dirs, key=lambda pair: pair[1])
bands_dirs = [s[0] for s in bands_dirs]

def dynfocBand(bandidx): 
    BMP_FILES = list(bands_dirs[bandidx].glob("*"))
    sorted_BMP_FILES = [(b, int(b.stem)) for b in BMP_FILES ]
    sorted_BMP_FILES = sorted(sorted_BMP_FILES, key=lambda pair: pair[1])
    sorted_BMP_FILES = [s[0] for s in sorted_BMP_FILES]
    
    for imgidx, imgpath in enumerate(sorted_BMP_FILES):
        img = Image.open(str(imgpath)).convert('L')
        
        for i in range(N_FOCUSDEPTHS):
            snippetidx = imgidx + i # local to each band
            
            x1, y1, x2, y2 = 0, 0+(i*CROP_HEIGHT), CROP_WIDTH, CROP_HEIGHT+(i*CROP_HEIGHT)
            box = (x1, y1, x2, y2)
            crop_img = img.crop(box)
            newim = Image.new('L', (CROP_WIDTH, CROP_HEIGHT))
            newim.paste(crop_img, (0,0))
            newim_arr = np.array(newim)
            
            vol = cv2.Laplacian(newim_arr, cv2.CV_64F, ksize=1, borderType=cv2.BORDER_REFLECT).var()
            
            if(maxVolSnippetVols[bandidx*N_SNIPPETS + snippetidx] < vol):
                maxVolSnippetVols[bandidx*N_SNIPPETS + snippetidx] = vol
                maxVolSnippetImgs[bandidx*N_SNIPPETS + snippetidx] = newim

            snippetidx += 1

threads = []
for bandidx in range(len(bands_dirs)): 
    threads.append(threading.Thread(target=dynfocBand, args=(bandidx,)))

for thread in threads: thread.start() 
for thread in threads: thread.join()

# stitch it all together 
fullimg = Image.new("L", (CROP_WIDTH*N_BANDS, CROP_HEIGHT*N_SNIPPETS))
for bandidx in range(N_BANDS): 
    x1 = bandidx*CROP_WIDTH
    x2 = x1 + CROP_WIDTH
    y1 = 0
    y2 = CROP_HEIGHT
    for snippetidx in range(N_SNIPPETS): 
        fullimg.paste(maxVolSnippetImgs[bandidx*N_SNIPPETS + snippetidx], (x1,y1,x2,y2))
        y1 += CROP_HEIGHT
        y2 += CROP_HEIGHT

fullimg.save(f'{job_name}.png')