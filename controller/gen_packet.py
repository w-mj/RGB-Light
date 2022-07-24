import socket
import struct

addr = "192.168.101.239"
# addr = "192.168.222.1"
port = 4617

TAG_HELLO=1
TAG_SET=2
TAG_GET=3
TAG_SET_FRAME=5
TAG_SAVE=6
TAG_RESTART=8
TAG_NVS_COMMIT=9
TAG_GET_INFO=10


def get_packet(tag, key, value):
    if (tag in (TAG_HELLO, TAG_NVS_COMMIT, TAG_GET_INFO)):
        return struct.pack("=b", tag)
    format = f"=bb{len(key)}sb{len(value)}s"
    return struct.pack(format, tag, len(key), key.encode(), len(value), value.encode())


s = socket.socket()
s.connect((addr, port))

# s.send(get_packet(TAG_SET, "sta_ssid", "TP-LINK_DE06FA"))
# s.send(get_packet(TAG_SET, "sta_password", "G313G313G313"))
# s.send(get_packet(TAG_NVS_COMMIT, "", ""))

def get_info():
    pack = get_packet(TAG_GET_INFO, None, None)
    s.send(pack)
    format = "I"
    data = s.recv(struct.calcsize(format))
    # print(data)
    info_pack = struct.unpack(format, data)
    print(f"free heap size: {info_pack[0]}")

get_info()
