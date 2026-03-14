import serial
import serial.tools.list_ports
from enum import Enum
import time

from command_io import CommandIo
from animation_streamer import AnimationStreamer

class State(Enum):
    WAITING = 1
    SENDING_METADATA = 2
    PLAYING_ANIMATION = 3

enable_printing = True
animation_file = "media/animation.bin"
frame_rate = 25 # fps

posx = 0 
posy = 0

cmd_io = None
animation_streamer = None
ser = None

def printif(data):
    if enable_printing:
        print(data)

def command_state_machine():
    state = State.WAITING
    frame_start = 0 

    while (True):
        command, args = cmd_io.read_command()
        if command == "error_command":
            continue

        match command:
            case "animation_request":
                if state == State.WAITING:
                    state = State.SENDING_METADATA

            case "set_orientation":
                animitation_streamer.upside_down = (args[0] > 0)


            case "frame_request":
                if state == State.PLAYING_ANIMATION:
                    animation_streamer.start_frame_stream()
                    if animation_streamer.is_done: 
                        state = State.WAITING

            case "next_frame_packet":
                animation_streamer.stream_next_fb_backet()

            case "cancel_animation":
                state = State.WAITING

        match state:

            # Send metadata before starting animation
            # Framerate and location can be changed in-between frames
            case State.SENDING_METADATA:
                cmd_io.write_command("set_framerate", [frame_rate, 0])
                cmd_io.write_command("set_location", [posx, posy])
                cmd_io.write_command("start_animation", [animation_streamer.width, animation_streamer.height])
                state = State.PLAYING_ANIMATION

if __name__ == '__main__':
    cmd_io = CommandIo(None)
    while True:
        try:
            port = next(serial.tools.list_ports.grep("USB serial display"), None)
            if (port is None): raise serial.SerialException

            # It's important that there is no serial timeout
            with serial.Serial(port.device, 115200, timeout=(cmd_io.config.ack_timout_ms / 1000), write_timeout=0) as ser:
                cmd_io.serial = ser
                animation_streamer = AnimationStreamer(animation_file, cmd_io) 
                command_state_machine()

        except serial.SerialException:
            print("No connection, retrying")
            time.sleep(1)

