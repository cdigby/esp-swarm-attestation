
import socket, struct

HOST = "192.168.4.1"
PORT = 3333

DATA = struct.pack("BB12s", 0x01, 12, b'TEST_CLIENT\0')

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print("connecting...")
    s.connect((HOST, PORT))
    print("sending data...")
    s.send(DATA)
    print("done")

