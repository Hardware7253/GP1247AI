
from PIL import Image, ImageSequence
import sys
import os
import shutil

if __name__ == '__main__':
    in_file = "media/smile.png"
    out_file = "image.h"

    # First argument input file
    if (len(sys.argv) > 1):
        gif_file = sys.argv[1]

    # Second argument output file
    if (len(sys.argv) > 2):
        out_file = sys.argv[2]

    # Convert gif frames to raw frame buffer byte data
    # The first 4 bytes contain the width then the height as little endian 16 bit numbers
    with open(out_file, "wb") as f:
        img = Image.open(in_file).convert("1")
        img.save(out_file, format="XBM")   