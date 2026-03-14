import serial
import serial.tools.list_ports

port = next(serial.tools.list_ports.grep("USB serial display"))
serial = serial.Serial(port.device, 115200)

while True:
    # serial.write("PING".encode("utf-8"))
    print(serial.read(4))
    serial.write("SF00000000".encode("utf-8"))

# import serial
# import serial.tools.list_ports
# import time

# port = next(serial.tools.list_ports.grep("USB serial display"))
# ser = serial.Serial(port.device, 115200, timeout=0)

# while True:
#     ser.write(b"PING")
#     data = b""
#     while len(data) < 4:
#         if ser.in_waiting:
#             data += ser.read(ser.in_waiting)
#     print(data)
