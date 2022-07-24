import itertools
import json
import os
import socket
import ipaddress
import struct
from time import sleep

import ifaddr
from tqdm import tqdm
from concurrent.futures import ThreadPoolExecutor


TAG_HELLO=1
TAG_SET=2
TAG_GET=3
TAG_SET_FRAME=5
TAG_SAVE=6
TAG_CHANGE_MODE=7
TAG_RESTART=8
TAG_NVS_COMMIT=9
TAG_GET_INFO=10

target_port = 4617


def get_packet(tag, key, value):
    if tag in (TAG_HELLO, TAG_NVS_COMMIT, TAG_GET_INFO, TAG_RESTART, TAG_SAVE):
        return struct.pack("=b", tag)
    if tag == TAG_SET:
        if key in ('ms_per_frame', 'total_frames'):
            f = f"=bb{len(key)}si"
            return struct.pack(f, tag, len(key), key.encode(), value)
        else:
            f = f"=bb{len(key)}sb{len(value)}s"
            return struct.pack(f, tag, len(key), key.encode(), len(value), value.encode())
    if tag == TAG_SET_FRAME:
        frame = value
        byte_array = []
        for led in frame:
            r = (led >> 16) & 0xff
            g = (led >> 8 ) & 0xff
            b = (led >> 0 ) & 0xff
            byte_array.append(r)
            byte_array.append(g)
            byte_array.append(b)
        return struct.pack("=bB180s", tag, key, bytearray(byte_array))
    if tag == TAG_CHANGE_MODE:
        return struct.pack("=bb", tag, key)


def check_host(host):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(2)
    result = sock.connect_ex((host, target_port))
    sock.close()
    return result == 0


def scan_network(ip, net_mask):
    me = ipaddress.IPv4Address(ip)
    if me.is_loopback or me.is_link_local or me.is_global:
        return

    network = ipaddress.ip_network(f"{ip}/{net_mask}", False)
    print(f"扫描网段 {network}...")
    # print(f"me.is_global: {me.is_global}")
    # print(f"me.is_link_local: {me.is_link_local}")
    # print(f"me.is_multicast: {me.is_multicast}")
    # print(f"me.is_private: {me.is_private}")
    # print(f"me.is_unspecified: {me.is_unspecified}")
    # pool = multiprocessing.Pool(40)
    pool = ThreadPoolExecutor(max_workers=40)
    hosts = [str(x) for x in network.hosts() if x != me]
    result = pool.map(check_host, hosts)
    result = [x[0] for x in zip(hosts, result) if x[1]]
    print(result)
    return result


def scan():
    adapters = ifaddr.get_adapters()
    adapters = [x for x in adapters if 'Virtual' not in x.nice_name]
    adapter_ips = [adapter.ips for adapter in adapters]
    ips = list(itertools.chain(*adapter_ips))
    ips = [(ip.ip, ip.network_prefix) for ip in ips if ip.is_IPv4]
    # ips = [x for x in ips if x[0].startswith("192.168")]
    print(ips)
    for addr in ips:
        result = scan_network(addr[0], addr[1])
        if result:
            return result[0]
    return None


def set_wifi(target_host):
    ssid = input("请输入WIFI SSID")
    password = input("请输入WIFI密码")
    s = socket.socket()
    s.connect((target_host, target_port))
    if ssid:
        s.send(get_packet(TAG_SET, "sta_ssid", ssid))
    if password:
        s.send(get_packet(TAG_SET, "sta_password", password))
    s.close()


def set_effect(target_host):
    files = [x for x in os.listdir() if x.startswith("effect_") and x.endswith(".json")]
    if not files:
        print("未发现灯效文件")
        return
    for f in files:
        effect_name = f.replace("effect_", "").replace(".json", "")
        print(effect_name)
    while True:
        effect_name = input("请输入灯效名")
        effect_file = f"effect_{effect_name}.json"
        if os.path.exists(os.path.join(os.getcwd(), effect_file)):
            break
        print(f"未找到灯效文件：{effect_file}")
    effect = json.load(open(os.path.join(os.getcwd(), effect_file)))
    print(f"打开灯效文件：{effect_file}")
    print(f"每帧持续时间：{effect['ms_per_frame']} ms")
    print(f"全部帧数：{len(effect['frames'])}")

    s = socket.socket()
    s.connect((target_host, target_port))
    s.send(get_packet(TAG_SET, "total_frames", effect['total_frames']))
    s.send(get_packet(TAG_SET, "ms_per_frame", effect['ms_per_frame']))
    for i, frame in tqdm(enumerate(effect['frames'])):
        pkg = get_packet(TAG_SET_FRAME, i, frame);
        s.send(pkg)
        s.recv(1)
    s.send(get_packet(TAG_SAVE, None, None))


def reboot(target_host):
    s = socket.socket()
    s.connect((target_host, target_port))
    s.send(get_packet(TAG_RESTART, None, None))
    s.close()


def effect_setter(effect):
    def setter(target_host):
        s = socket.socket()
        s.connect((target_host, target_port))
        s.send(get_packet(TAG_CHANGE_MODE, effect, None))
        s.close()
    return setter


def main():
    config = {}
    if os.path.exists("config.json"):
        config = json.load(open("config.json"))
    target_host = config.get('target_host', None)

    if target_host and check_host(config['target_host']):
        print(f"使用灯带控制器：{target_host}")
    else:
        target_host = None

    if not target_host:
        print("正在扫描...")
        target_host = scan()
        # target_host = "192.168.101.139"
        if target_host:
            print(f"找到灯带控制器 {target_host}")
            config['target_host'] = target_host
            json.dump(config, open('config.json', 'w'))
        else:
            print("未找到灯带控制器")
            exit(1)

    op_list = [set_wifi, set_effect, reboot, effect_setter(0), effect_setter(1), effect_setter(2), effect_setter(3)]
    while True:
        print("==================================")
        print("|请输入操作序号：")
        print("|\t 1) 设置WIFI参数")
        print("|\t 2) 导入灯效文件")
        print("|\t 3) 重启等效控制器")
        print("|\t 4) 进入自定义灯效模式")
        print("|\t 5) 进入时间模式")
        print("|\t 6) 进入显示IP模式")
        print("|\t 7) 进入流水灯模式")
        print("|\t 0) 退出程序")
        print("==================================")
        command = input()
        try:
            command = int(command)
        except ValueError:
            command = -1
        if 1 <= command <= len(op_list) + 1:
            op_list[command - 1](target_host)
        elif command == 0:
            break
        else:
            print("输入的操作序号无效")


if __name__ == '__main__':
    main()


