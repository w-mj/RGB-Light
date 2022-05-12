import 'dart:developer';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:network_tools/network_tools.dart';
import 'package:tcp_scanner/tcp_scanner.dart';

const commPort = 4617;

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
  List<DropdownMenuItem<String>> dropDownList = [];
  List<Device> devices = [];
  String deviceListValue = "default";

  DeviceScanState() {
    dropDownList
        .add(const DropdownMenuItem(value: "default", child: Text("未选择")));
  }

  @override
  Widget build(BuildContext context) {
    return Flex(
      direction: Axis.horizontal,
      children: [
        const Padding(padding: EdgeInsets.symmetric(horizontal: 20)),
        Expanded(
          flex: 1,
          child: DropdownButton(
            value: deviceListValue,
            items: dropDownList,
            onChanged: (value) {
              setState(() {
                deviceListValue = value.toString();
              });
            },
          ),
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
          devices.add(Device(event.ip, event.port));
          setState(() {
            dropDownList
                .add(DropdownMenuItem(value: event.ip, child: Text(event.ip)));
            if (dropDownList.length == 2 &&
                dropDownList[0].value == 'default') {
              deviceListValue = event.ip;
              dropDownList.removeAt(0);
            }
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
        if (dropDownList.length == 1 && dropDownList[0].value == 'default') {
          setState(() {
            dropDownList[0] =
                const DropdownMenuItem(value: "default", child: Text("无设备"));
          });
        }
      }
    });
  }

  void getIps() {
    setState(() {
      dropDownList.clear();
      dropDownList
          .add(const DropdownMenuItem(value: "default", child: Text("正在扫描")));
      deviceListValue = 'default';
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
  }
}
