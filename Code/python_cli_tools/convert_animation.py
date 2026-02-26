from PIL import Image, ImageSequence
import argparse
import sys
import os
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
    fit_mode: FitMode = FitMode.LETTERBOX,
):
    """Convert a video file to a gif to be displayed"""

    vf = ()
    match fit_mode:
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
        case _:
            raise Exception(f"Unknown fitmode {fit_mode}")

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


def convert_from_gif(gif_file: str, out_file: str, config: config.Config, fps: int, location: tuple[int, int]):
    """
        Converts from gif / image to binary for streaming to display
        GIF will produce multiple frames in the binary for streaming
        PNG will produce a single frame
    """

    meta_num_length = config.num_byte_len
    with open(out_file, "wb") as f:

        with Image.open(gif_file) as gif:

            # Write width, height, number of frames, and frame rate metadata to the start of the file
            first_frame = next(ImageSequence.Iterator(gif))
            width, height = first_frame.size
            f.write(width.to_bytes(meta_num_length, 'little'))
            f.write(height.to_bytes(meta_num_length, 'little'))

            posx, posy = location
            f.write(posx.to_bytes(meta_num_length, 'little'))
            f.write(posy.to_bytes(meta_num_length, 'little'))

            f.write(sum(1 for _ in ImageSequence.Iterator(gif)).to_bytes(meta_num_length, 'little')) # Number of frames
            f.write(fps.to_bytes(meta_num_length, 'little'))

            # Seek to end of metadata incase the meta length is longer than the actual metadata
            f.seek(config.meta_len * config.num_byte_len, 0)

            for i, frame in enumerate(ImageSequence.Iterator(gif)):
                bmp = frame.convert('1').tobytes("raw")
                bmp = swap_bit_order(bmp)

                if not config.bmp_row_major:
                    bmp = transpose_bitmap(bmp, width, height)

                if not config.bmp_lsb_first:
                    bmp = swap_bit_order(bmp)
                
                f.write(bmp)

def main():
    cfg = config.Config()

    # defaults
    in_file = ""
    out_file = ""
    fps = 25
    pos = (0, 0)
    size_x = 252
    size_y = 64
    fit_mode = FitMode.LETTERBOX
    resize = False

    parser = argparse.ArgumentParser(
        description=(
            "Animation converter CLI app that outputs a binary "
            "that can be uploaded to the display. "
            "Only input and output path arguments are required, other arguments have default values."
        ),
        formatter_class=lambda prog: argparse.HelpFormatter(
            prog,
            max_help_position=40
        )
    )

    parser.add_argument(
        "-i", "--input",
        type = str,
        required = True,
        metavar = "FILE_PATH",
        help = "Input file path, supports mp4, mkv, png, jpg, jpeg, and gif. \
            Non gif files are converted to gifs, this gif will be stored at (input_file).gif"
    )

    parser.add_argument(
        "--pos",
        nargs = 2,
        type = int,
        metavar = ("X", "Y"),
        help = "X and Y position to display the bitmap at on the display"
    )

    parser.add_argument(
        "--fps",
        type = int,
        help = "The display will run at this fps, and videos will be converted to this fps"
    )

    parser.add_argument(
        "--resize",
        nargs = 2,
        type = int,
        metavar = ("Width", "Height"),
        help = "Dimensions to resize the input file to. Does not work for image formats"
    )

    parser.add_argument(
        "--resize-mode", 
        type = str,
        help = "Set to either letterbox, crop, or stretch"
    )

    parser.add_argument(
        "-o", "--output",
        type = str,
        required = True,
        metavar = "FILE_PATH",
        help = "Output file path for the animation binary"
    )
    
    args = parser.parse_args()
    
    if args.pos:
        pos = args.pos

    if args.fps:
        fps = args.fps
    
    if args.input:
        in_file = args.input

    if args.output:
        out_file = args.output

    if args.resize:
        resize = True
        size_x, size_y = args.resize

    if args.resize_mode:
        mode = args.resize_mode
        if mode == "letterbox":
            fit_mode = FitMode.LETTERBOX
        elif mode == "crop":
            fit_mode = FitMode.CROP
        elif mode == "stretch":
            fit_mode = FitMode.STRETCH
        else:
            raise Exception("Invalid resize mode")

    # Convert in file to gif
    gif_file = in_file
    if in_file.lower().endswith((".gif")):
        if resize:
            gif_out = in_file + ".resized.gif"
            video_to_gif(in_file, gif_out, size_x, size_y, fps, fit_mode)
            gif_file = gif_out 
            

    else:
        gif_out = in_file + ".gif"
        if in_file.lower().endswith((".mp4", ".mkv")):
            video_to_gif(in_file, gif_out, size_x, size_y, fps, fit_mode)
            gif_file = gif_out 
        elif in_file.lower().endswith((".png", ".jpg", ".jpeg")):
            pass
        else:
            raise Exception("Filetype not supported")

    # Convert gif to animation binary
    convert_from_gif(gif_file, out_file, cfg, fps, pos)

if __name__ == '__main__':
    main()
