import socket
import struct

addr = "192.168.222.1"
port = 4617

TAG_HELLO=1
TAG_SET=2
TAG_GET=3
TAG_ADD_EFFECT=5
TAG_DEL_EFFECT=6
TAG_SET_EFFECT=7
TAG_RESTART=8
TAG_NVS_COMMIT=9

def get_packet(tag, key, value):
    if (tag == TAG_HELLO or tag == TAG_NVS_COMMIT):
        return struct.pack("=b", tag)
    format = f"=bb{len(key)}sb{len(value)}s"
    return struct.pack(format, tag, len(key), key.encode(), len(value), value.encode())

s = socket.socket()
s.connect((addr, port))
# s.send(get_packet(TAG_SET, "sta_ssid", "iQOO Neo5"))
# s.send(get_packet(TAG_SET, "sta_password", "qazplm0987111"))
s.send(get_packet(TAG_SET, "sta_ssid", "TP-LINK_DE06FA"))
s.send(get_packet(TAG_SET, "sta_password", "G313G313G313"))
s.send(get_packet(TAG_NVS_COMMIT, "", ""))
