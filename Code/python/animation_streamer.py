import os
import command_io

class AnimationStreamer:
    def __init__(self, filename, cmd_io: command_io.CommandIo, repeat=True): 
        """
            Initialise animation class
            animation will continue sending over serial until program is interrupted if repeat=True
        """

        self.filename = filename
        self.frame_idx = 0
        self.repeat = repeat
        num_length = cmd_io.config.num_byte_len
        self.meta_length = num_length * 2 
        self.is_done = False
        self.cmd_io = cmd_io

        self.tmp = 0

        # Used for sending fb packets
        self.frame_buffer = b''
        self.fb_index = 0

        # Load metadata from animation binary
        with open(self.filename, "rb") as f:
            meta_bytes = f.read(self.meta_length)

            self.width = int.from_bytes(meta_bytes[:num_length], 'little', signed=False)
            self.height = int.from_bytes(meta_bytes[num_length:self.meta_length], 'little', signed=False)

            if cmd_io.config.bmp_row_major:
                self.bytes_per_frame = ((self.width + 7) // 8) * self.height
            else:
                self.bytes_per_frame = ((self.height + 7) // 8) * self.width

        size = os.path.getsize(self.filename)
        self.frames = (size - self.meta_length) // self.bytes_per_frame

    def start_frame_stream(self):
        """
            Reads the next frame bitmap from the animation file and stores it in self.framebuffer
        """

        if self.frame_idx >= self.frames:
            if self.repeat:
                self.frame_idx = 0
            else:
                self.is_done = True
                return

        with open(self.filename, "rb") as f:
            f.seek(self.meta_length + (self.bytes_per_frame * self.frame_idx))
            self.frame_buffer = f.read(self.bytes_per_frame)
            self.fb_index = 0

    def stream_next_fb_backet(self):
        """
            Streams the next packet of the current frame
            start_frame_stream needs to be called before using this function
        """
        packet_start = self.fb_index
        packet_end = self.fb_index + self.cmd_io.config.packet_size 

        packet_len = packet_end - packet_start

        packet_extend = 0
        if packet_end >= len(self.frame_buffer): 
            self.frame_idx += 1;
            packet_end = len(self.frame_buffer) # Ensure we don't index outside the bitmap

            # Calculate how many bytes to keep the correct packet size 
            packet_extend = self.cmd_io.config.packet_size - (packet_end - packet_start) 

        packet = self.frame_buffer[packet_start : packet_end] + b'\x00' * packet_extend

        # print(f"tx packet {self.tmp}, raw_len: {packet_len}, extend: {packet_extend}")
        self.cmd_io.write_buf(packet)
        self.tmp += 1;
        self.fb_index += packet_end - packet_start;

    