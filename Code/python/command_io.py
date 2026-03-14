import serial
import serial.tools.list_ports
from config import Config

class CommandIo():
    def __init__(self, serial):
        self.serial = serial
        self.config = Config()
        self.last_command = "error_command"
        self.tmp = 0
        # self.commands = list(self.config.commands_dict.keys())

    def write_buf(self, buf, enable_ack=True):
        """
            Writes a byte buffer
            If enable_ack=Ture this function will block until ACK command has been recieved from the device
        """
        if (len(buf) == 0):
            print("bp")

        print(f"Writing buf {buf[0]} with len {len(buf)}, enable ack: {enable_ack}")

        if not enable_ack:
            self.serial.write(buf)
            return

        command = "error_command"

        # This usually waits for an acknowledge
        # But may also be broken by a normal command incase the acknowledge was missed
        while (command == "error_command"):
        # while (not command == "acknowledge"):
            self.serial.write(buf)
            command, args = self.read_command(False)



    def write_command(self, name, args, enable_ack=True):
        """
            Writes the command string and arguments.
            Arguments should be given as an array of numbers.
            If enable_ack=Ture this function will block until ACK command has been recieved from the device
        """
        # print(f"write command {name}, args {args}")
        buf = bytearray()
        buf += self.config.commands_dict[name].encode("utf-8")

        if name == "acknowledge" and self.last_command == "acknowledge":
            print("Wtf")

        for arg in args:
            buf += arg.to_bytes(self.config.num_byte_len, 'little')

        self.write_buf(buf, enable_ack)

    def read_command(self, enable_ack=True):
        """
            Reads a command 
            Timeout is specified when setting up serial
            a tuple is returned with (the name of the command, and the arguments array)
        """
        cmd_len = self.config.message_chars + (self.config.max_arguments * self.config.num_byte_len)
        message = self.serial.read(cmd_len) 

        # Handle timout / wrong number of bytes
        # test = len(message)
        # if test > 0:
        #     print(f"Hello {test}")

        if len(message) != cmd_len:
            self.serial.flushInput()
            return ("error_command", [0, 0])

        message_id = message[:self.config.message_chars].decode("utf-8")

        args = []
        for arg in range(self.config.max_arguments):
            data = message[self.config.message_chars + (arg * self.config.num_byte_len) : ((arg + 1) * self.config.num_byte_len)]
            args.append(int.from_bytes(data, byteorder='little', signed=False))

        swapped_commands_dict = dict((value, key) for key, value in self.config.commands_dict.items())

        if (message_id in swapped_commands_dict):
            command_name = swapped_commands_dict[message_id]
            self.last_command = command_name
            self.tmp += 1
            print(f"I recieved {command_name}, ack enabled {enable_ack} ({self.tmp})")
            if enable_ack: self.write_command("acknowledge", [0, 0], False) 
        else:
            command_name = "error_command"
        return(command_name, args)
    
# For testing
if __name__ == '__main__':
    port = next(serial.tools.list_ports.grep("USB serial display"))
    serial = serial.Serial(port.device, 115200)

    cmd_io = CommandIo(serial)
    # cmd_io.write_command("start_animation", [1, 2])

    # counter = 0
    # while True:
    #     print(cmd_io.read_command())
    #     counter += 1
    #     print(counter)
