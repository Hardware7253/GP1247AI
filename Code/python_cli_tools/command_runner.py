import serial
import serial.tools.list_ports
from datetime import datetime
import argparse

from command_io import CommandIo
from file_streamer import FileStreamer 

def file_stream_command_runner(file_streamer: FileStreamer, cmd_io: CommandIo):
    while (not file_streamer.stream_done):

        # Read command and manually send back acknowledge
        command, args = cmd_io.read_command(False)
        if command == "error_command":
            continue

        # Don't send acknowledge for request next packet as the packet itself acts as the acknowledge
        if command != "request_next_packet":
            cmd_io.write_command("acknowledge", enable_ack=False)

        match command:
            case "request_next_packet":
                file_streamer.stream_next_packet()
            case "send_erase_progress":
                print(f"Erasing flash ({args[0]}%)              ", end="\r")

def set_rtc_time(cmd_io: CommandIo):

    # Wait until the start of a second to synch the time
    # Because the device HAL always sets subsecond to 0.0 when the time is loaded
    now = datetime.now()
    while(now.microsecond > 1):
        now = datetime.now()

    cmd_io.write_command("send_hour", [now.hour])
    cmd_io.write_command("send_minute", [now.minute])
    cmd_io.write_command("send_second", [now.second])
    cmd_io.write_command("set_time")

    cmd_io.write_command("send_year", [now.year])
    cmd_io.write_command("send_month", [now.month])
    cmd_io.write_command("send_day", [now.day])
    cmd_io.write_command("send_weekday", [now.isoweekday()])
    cmd_io.write_command("set_date")

def get_rtc_time(cmd_io: CommandIo):
    """ Returns a tuple containg the rtc time in the format (hours, minutes, seconds, subseconds)"""
    cmd_io.write_command("request_time")
    hour = 0
    minute = 0
    second = 0
    subsecond = 0

    command = "error_command"
    while command != "set_time":
        command, args = cmd_io.read_command(True)
        match command:
            case "send_hour":
                hour = args[0]

            case "send_minute":
                minute = args[0]

            case "send_second":
                second = args[0]

            case "send_subsecond":
                subsecond = args[0]

    print(f"System time: {datetime.now()}")
    print(f"RTC time: {hour:02d}:{minute:02d}:{second:02d}.{subsecond:02d}")


def main():
    parser = argparse.ArgumentParser(
        description=(
            "This is a CLI tool for communicating with the display. "
        ),
        formatter_class=lambda prog: argparse.HelpFormatter(
            prog,
            max_help_position=40 
        )
    )

    parser.add_argument(
        "-u", "--upload-animation",
        type = str,
        metavar = "FILE_PATH",
        help="provide an animation binary file path to upload it to the display. Large files can take up to 3 minutes to upload"
    )

    parser.add_argument(
        "-s", "--set-dtime",
        action="store_true",
        help="sets the date and time on the devices RTC to the system date and time"
    )

    parser.add_argument(
        "-r", "--read-time",
        action="store_true",
        help="reads the time from the devices RTC and prints it alongside the system time"
    )

    parser.add_argument(
        "--cal-rtc-forward",
        type = int,
        metavar = "PPM",
        help = "adds the PPM calibration value to the devices RTC to make it run faster"
    )

    parser.add_argument(
        "--cal-rtc-backward",
        type = int,
        metavar = "PPM",
        help = "adds the PPM calibration value to the devices RTC to make it run slower"
    )

    args = parser.parse_args()

    cmd_io = CommandIo(None)
    port = next(serial.tools.list_ports.grep("USB serial display"), None)
    if (port is None): 
        raise Exception("The display could not be found, is it connected?")

    # It's important that there is no serial timeout
    with serial.Serial(port.device, 115200, timeout=(cmd_io.config.ack_timout_ms / 1000), write_timeout=0) as ser:
        cmd_io.serial = ser

        if args.upload_animation:
            file_streamer = FileStreamer(args.upload_animation, cmd_io) 
            file_stream_command_runner(file_streamer, cmd_io)

        if args.set_dtime:
            set_rtc_time(cmd_io)

        if args.read_time:
            get_rtc_time(cmd_io)

        if args.cal_rtc_forward:
            cmd_io.write_command("cal_rtc_forward", [args.cal_rtc_forward])

        if args.cal_rtc_backward:
            cmd_io.write_command("cal_rtc_backward", [args.cal_rtc_backward])


if __name__ == '__main__':
    main()
