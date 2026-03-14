from PIL import Image, ImageSequence
import sys
import os
import shutil
import subprocess
from enum import Enum

import config

class FitMode(Enum):
    LETTERBOX = 1
    CROP = 2
    STRETCH = 3

def video_to_gif(
    input_path,
    output_path,
    width: int,
    height: int,
    fps=25,
    fitmode: FitMode = FitMode.STRETCH,
):
    """Convert a video file to a gif to be displayed"""

    vf
    match fitmode:
        case FitMode.LETTERBOX:
            vf = (
                f"fps={fps},"
                f"scale={width}:{height}:force_original_aspect_ratio=decrease,"
                f"pad={width}:{height}:(ow-iw)/2:(oh-ih)/2"
            )
        case FitMode.CROP:
            vf = (
                f"fps={fps},"
                f"scale={width}:{height}:force_original_aspect_ratio=increase,"
                f"crop={width}:{height}"
            )
        case FitMode.STRETCH:
            vf = (
                f"fps={fps},"
                f"scale={width}:{height}"
            )

    cmd = [
        "ffmpeg",
        "-y",
        "-i", str(input_path),
        "-vf", vf,
        str(output_path),
    ]

    subprocess.run(cmd, check=True)

def reverse_bits(byte):
    out = 0
    for i in range(8):
        out |= ((byte & (1 << i)) != 0) << (7 - i)
    return out

def swap_bit_order(byte_array: bytearray):
    """
        Swaps the bit order of all bytes in a byte array
    """
    new_bytearray = bytearray()
    for byte in byte_array:
        new_bytearray.append(reverse_bits(byte))
    return new_bytearray

def flip_bmp(bmp: bytearray, is_row_major: bool):
    """
        Returns a new flipped bitmap so it displays upside down
    """
    new_bmp = bytearray(len(bmp))
    
    for i, byte in enumerate(bmp):
        new_bmp[len(bmp) - i - 1] = reverse_bits(bmp[i])
    return new_bmp

def transpose_bitmap(bmp: bytearray, bmp_width: int, bmp_height: int):
    """
        Transpose a row major LSB first bitmap to a
        column major LSB first mitmap
    """

    new_bitmap = bytearray()

    # Get bitmap length
    bmp_aligned_width = (bmp_width + 7) // 8
    bmp_aligned_height = (bmp_height + 7) // 8
    bmp_len = bmp_aligned_width * bmp_height

    # Transpose
    for x in range(bmp_width):
        y_bytes = bytearray(bmp_aligned_height)
        x_div = x // 8 # Byte position of the current x coordinate in it's row
        x_mod = x % 8 # The bit the current x coordinate occupies in it's row byte
        for y in range(bmp_height):
            bit = (bmp[(y * bmp_aligned_width) + x_div] & (1 << x_mod)) > 0
            y_bytes[y // 8] |= (bit << (y % 8))
        new_bitmap.extend(y_bytes)

    return new_bitmap


def convert_from_gif(gif_file, out_file, config: config.Config):
    """
        Converts from gif / image to binary for streaming to display
        GIF will produce multiple frames in the binary for streaming
        PNG will produce a single frame
    """

    meta_num_length = config.num_byte_len
    with open(out_file, "wb") as f:

        with Image.open(gif_file) as gif:

            # Write width and height metadata to the start of the file
            first_frame = next(ImageSequence.Iterator(gif))
            width, height = first_frame.size
            f.write(width.to_bytes(meta_num_length, 'little'))
            f.write(height.to_bytes(meta_num_length, 'little'))

            for i, frame in enumerate(ImageSequence.Iterator(gif)):
                # f.write(convert_frame_to_bmp(frame, config.bmp_row_major, config.bmp_lsb_first))
                bmp = frame.convert('1').tobytes("raw")
                bmp = swap_bit_order(bmp)

                if not config.bmp_row_major:
                    bmp = transpose_bitmap(bmp, width, height)

                if not config.bmp_lsb_first:
                    bmp = swap_bit_order(bmp)
                
                f.write(bmp)

if __name__ == '__main__':

    # defaults
    in_file = "media/smile.gif"
    out_file = "media/animation.bin"
    cfg = config.Config()

    # First argument input file
    if (len(sys.argv) > 1):
        in_file = sys.argv[1]

    # Second argument output file
    if (len(sys.argv) > 2):
        out_file = sys.argv[2]

    # Convert in file to gif
    gif_file = in_file
    if not in_file.lower().endswith((".gif")):
        gif_out = in_file + ".gif"
        if in_file.lower().endswith((".mp4", ".mkv")):
            video_to_gif(in_file, gif_out, 252, 62)
            gif_file = gif_out 
        elif in_file.lower().endswith((".png", ".jpg", ".jpeg")):
            pass
        else:
            raise Exception("Filetype not supported")

    convert_from_gif(gif_file, out_file, cfg)