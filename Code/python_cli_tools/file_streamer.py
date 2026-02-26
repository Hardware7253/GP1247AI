import os
import command_io

class FileStreamer:
    def __init__(self, filename: str, cmd_io: command_io.CommandIo): 
        self.filename = filename
        self.cmd_io = cmd_io
        self.byte_idx = 0
        self.packet_size = cmd_io.config.packet_size
        self.file_size = os.path.getsize(filename)
        self.stream_done = False;

        self.progress = 0

        if self.file_size > cmd_io.config.device_storage_bytes:
            raise Exception(f"File to big to send to device ({self.file_size}B > {cmd_io.config.device_storage_bytes}B)")

        cmd_io.write_command("start_file_transfer", [self.file_size])

    def stream_next_packet(self):
        """
            Streams the next packet in the file
        """

        with open(self.filename, "rb") as f:
            if self.byte_idx >= self.file_size:
                raise Exception("Reached the end of the file")

            f.seek(self.byte_idx)
            packet_bytes = f.read(self.packet_size)
            self.byte_idx += len(packet_bytes)

            self.progress = (self.byte_idx * 100) // self.file_size
            print(f"writing flash ({self.progress}%)               ", end="\r")

            # Pad the end of the packet with 0's if the file is done
            pad_bytes = self.packet_size - len(packet_bytes)
            packet_bytes += b'\x00' * pad_bytes 

            if pad_bytes > 0:
                self.stream_done = True
                print("")

            self.cmd_io.write_buf(packet_bytes)