#!/usr/bin/env python3

import os
from pathlib import Path
import glob
import sys
from PIL import Image
import numpy as np


def normalize(arr):
    """
    Linear normalization
    http://en.wikipedia.org/wiki/Normalization_%28image_processing%29
    """
    arr = arr.astype('float')
    # Do not touch the alpha channel
    for i in range(3):
        minval = arr[...,i].min()
        maxval = arr[...,i].max()
        if minval != maxval:
            arr[...,i] -= minval
            arr[...,i] *= (255.0/(maxval-minval))
    return arr

def normalize_image(input: Image) -> Image:
    """
    Linear normalization
    http://en.wikipedia.org/wiki/Normalization_%28image_processing%29
    """
    arr = np.array(input)
    arr = arr.astype('float')
    return Image.fromarray(normalize(arr).astype('uint8'),'RGB')


def to_greyscale(input: str) -> str:
    # Inkplate 10 palette
    # palettedata = [0,0,0, 32,32,32, 64,64,64, 96,96,96, 128,128,128, 160,160,160, 192,192,192, 224,224,224]
    palettedata = [0,0,0, 36,36,36, 72,72,72, 109,109,109, 145,145,145, 182,182,182, 218,218,218, 255,255,255]

    palimage = Image.new("P", (16, 16))
    palimage.putpalette(palettedata * 32)
    # print(f"pal={palettedata}\n")

    # Load image and make greyscale
    im = Image.open(input).convert("RGB")
    im = normalize_image(im)

    qu = im.quantize(
        colors=8,
        method=Image.Quantize.MEDIANCUT,
        kmeans=0,
        palette=palimage,
        dither=Image.Dither.FLOYDSTEINBERG,
    )

    p = Path(input)
    output = p.with_suffix(".png")
    qu.save(output)
    # print(f"numColors = {len(qu.getcolors())} =  {qu.getcolors()}\n")
    return output

def to_bw(input: str) -> str:
    # Inkplate 10 palette
    palettedata = [0,0,0,  255,255,255]

    palimage = Image.new("P", (16, 16))
    palimage.putpalette(palettedata * 128)

    # Load image and make black and white
    im = Image.open(input).convert("RGB")

    qu = im.quantize(
        colors=2,
        method=Image.Quantize.MEDIANCUT,
        kmeans=0,
        palette=palimage,
        dither=Image.Dither.FLOYDSTEINBERG,
    )

    p = Path(input)
    output = p.with_suffix(".png")
    qu.save(output)
    return output


if __name__ == "__main__":
    # print(f"{len(sys.argv)} = {sys.argv}")
    if len(sys.argv) <= 1:
        print(f"Usage: {sys.argv[0]} file1 [file2 file3...]")
        sys.exit(0)

    for arg in sys.argv[1:]:
        for file in glob.glob(arg):
            if not os.path.exists(file):
                print(f"warning: file '{file}' not found")
                continue
            output = to_greyscale(file)
            # output = to_bw(file)
            print(f"  {file} -> {output}")
