import 'dart:developer';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:network_tools/network_tools.dart';

const commPort = 22;

class DeviceScan extends StatefulWidget {
  const DeviceScan({Key? key}) : super(key: key);

  @override
  State<StatefulWidget> createState() {
    return DeviceScanState();
  }
}

class Device {
  String ip;
  int port;
  String name = "";
  Device(this.ip, this.port) {
    name = ip;
  }
}

class DeviceScanState extends State<DeviceScan> {
  List<Device> devices = [];
  Device? currentDevice;
  String hint = "请点击扫描";

  @override
  Widget build(BuildContext context) {
    return Flex(
      direction: Axis.horizontal,
      children: [
        const Padding(padding: EdgeInsets.symmetric(horizontal: 20)),
        Expanded(
          flex: 1,
          child: (() {
            if (devices.isNotEmpty) {
              // currentDevice = devices[0];
              return DropdownButton<Device>(
                  value: currentDevice,
                  items: devices
                      .map((e) => DropdownMenuItem<Device>(
                          value: e, child: Text(e.name)))
                      .toList(),
                  onChanged: (Device? value) {
                    log(value!.name);
                    setState(() {
                      currentDevice = value;
                    });
                  });
            } else {
              return Text(hint);
            }
          }()),
        ),
        const Padding(padding: EdgeInsets.symmetric(horizontal: 10)),
        ElevatedButton(onPressed: getIps, child: const Text("扫描")),
        const Padding(padding: EdgeInsets.symmetric(horizontal: 20))
      ],
    );
  }

  int runninngTask = 0;
  void scan(subnet) {
    runninngTask++;
    log("start scan subnet $subnet");
    final stream =
        HostScanner.discover(subnet, firstSubnet: 1, lastSubnet: 255);
    stream.listen((host) {
      log("start scan port on ${host.ip}");
      PortScanner.discover(host.ip, startPort: commPort, endPort: commPort)
          .listen((event) {
        if (event.isOpen) {
          log("found $event ${event.ip} ${event.port}");
          setState(() {
            devices.add(Device(event.ip, event.port));
            currentDevice = devices[0];
          });
        }
      }, onDone: () {
        log('scan port on ${host.ip} finish');
      });
    }, onDone: () {
      log('Scan sub net $subnet finish');
      runninngTask--;
      if (runninngTask == 0) {
        log('all task finished');
        setState(() {
          if (devices.isEmpty) {
            hint = "无设备";
          } else {
            currentDevice = devices[0];
          }
        });
      }
    });
  }

  void getIps() {
    setState(() {
      devices.clear();
      hint = "正在扫描...";
    });
    devices.clear();
    NetworkInterface.list(type: InternetAddressType.IPv4).then((value) {
      Set<String> networks = <String>{};
      for (var element in value) {
        for (var addr in element.addresses) {
          final String ip = addr.address;
          final String subnet = ip.substring(0, ip.lastIndexOf('.'));
          if (networks.contains(subnet)) {
            continue;
          }
          networks.add(subnet);
          scan(subnet);
        }
      }
    });
    log("quit getIps");
  }
}
