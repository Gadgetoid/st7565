#!/usr/bin/python

import RPi.GPIO as GPIO
import time
import glob
import random
import numpy
import png
import math

W = 128
H = 64
PS = 8

IMG = []

def _bv(pos):
    return 1 << pos

def set_pixel(buf, x, y, color):
 
    page = y/PS
 
    offset = (page * W * 2) + (x*2)

    if color == 3: # Black 0/3         00
        buf[offset]     &= ~_bv(y%8)
        buf[offset + 1] &= ~_bv(y%8)
    elif color == 0: # Dark Grey 1/3   01
        buf[offset]     &= ~_bv(y%8)
        buf[offset + 1] |= _bv(y%8)
    elif color == 1: # Light Grey 2/3  10
        buf[offset]     |= _bv(y%8)
        buf[offset + 1] &= ~_bv(y%8)
    elif color == 2: # White 3/3       11
        buf[offset]     |= _bv(y%8)
        buf[offset + 1] |= _bv(y%8)

def load_png(fn):
    print("Loading PNG: {}".format(fn))
    f = png.Reader(file=open(fn))
    d = f.read()
    dat = list(d[2])
    buf = [0 for x in range(W*PS*2)]
    for x in range(128):
        for y in range(64):
            p = dat[y][x]
            set_pixel(buf, x, y, p)
    IMG.append(buf)


def main():
    files = glob.glob('*.png')
    for f in files:
        load_png(f)
    while True:
        for i in IMG:
            fifo = open('/tmp/st7565','w')  
            fifo.write("".join([chr(c) for c in i]))
            fifo.flush()
            fifo.close()
            time.sleep(1.0)

main()
