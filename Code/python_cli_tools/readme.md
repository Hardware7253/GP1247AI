# Python CLI Animation Uploader
The script `convert_animation.py` converts files like mp4, gif, png, etc to the 
uncompressed binary animation format used by the display.
Run `$ python convert_animation.py -h` for help using the animation converter CLI utility.

After converting the desired animation the command_runner can be used to upload it.
Run `$ python command_runner.py -u ANIMATION_BINARY_PATH` to upload an animation.
`$ python command_runner.py -s` can also be used to synch the devices clock to the PC clock.
Run `$ python command_runner.py -h` for help using the command_runner CLI utility.

## Example Usage
    $ python convert_animation.py -i media/test.gif -o media/animation.bin`
    $ python command_runner.py -u media/animation.bin -s`

## Calibration
The ppm calibration value can be calculated by muliplying the seconds drift over 24 hours by 11.574.
Run `$ python command_runner.py -h` for more information on reading time and calibration.

## Animation File Format
The animation file starts with 6 metadata numbers:
* Frame bitmap width (all frames are the same size)
* Frame bitmap height (all frames are the same size)
* x position of bitmap on the display
* y position of bitmap on the display
* Number of frames in the animation
* The animation fps

All metadata numbers are little endian; the byte length of the numbers is defined in `config.json`.
After the metadata bytes the frame bitmaps are stored back to back with no particular 
alignment with respect to byte addresses.
The frame bitmap format is defined in config.json; all bitmaps are either width or height byte aligned.

## Command Format
Currently `config.json` has all commands set to have only one argument.
Using four arguments would allow the send hour, minute, seconds, etc commands
to be combined into one. However, this wasn't done inorder to minimise the individual command length, thus
slightly speeding up operations that require frequent command transmission such as the file streamer.

## Config.py
Run `config.py` after the json file changes to re-generate the c header file for the embedded software.

## Python Dependencies
The following python libraries were used:
* PIL
* argparse
* sys 
* os 
* subprocess
* enum 
* serial
* json 
* datetime