#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import math

import scipy.signal
import scipy.interpolate

import numpy as np

import PIL
from PIL import Image

if __name__ == "__main__":
    parser = argparse.ArgumentParser();
    parser.add_argument("radius", type=int)
    parser.add_argument("-p", "--precision", type=int, default=256)
    parser.add_argument("-i", "--intensity", type=float, default=256)

    args = parser.parse_args()

    size = args.radius*2

    data = scipy.signal.parzen(args.precision*2+1)[args.precision:]

    interpolator = scipy.interpolate.interp1d(np.linspace(0, size/2, len(data)),
                                              data,
                                              kind="cubic")

    outdata = np.zeros((size, size))

    for y in range(size):
        for x in range(size):
            r = math.sqrt((x-size/2)**2+(y-size/2)**2)
            if r >= size/2:
                v = 0
            else:
                v = interpolator(r)

            outdata[y][x] = v

    norm = outdata.max()
    outdata /= norm

    outdata *= 255

    outdata = np.array(outdata, dtype=np.uint8)

    print(outdata)

    outimg = Image.fromarray(outdata, mode="L")
    outimg.save("test.png", "png")
