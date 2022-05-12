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
  int ledNum = 60;
  Device(this.ip, this.port) {
    name = "name$ip";
  }

  @override
  String toString() {
    return "$name $ip:$port $ledNum";
  }
}

class DeviceScanState extends State<DeviceScan> {
  List<Device> devices = [];
  Device? currentDevice;
  String hint = "请点击扫描";

  late TextEditingController nameController;
  late TextEditingController ledNumController;

  DeviceScanState() {
    nameController = TextEditingController(text: currentDevice?.name);
    ledNumController =
        TextEditingController(text: currentDevice?.ip.toString());
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Flex(
          direction: Axis.horizontal,
          children: [
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
                          nameController.text = currentDevice!.name;
                          ledNumController.text =
                              currentDevice!.ledNum.toString();
                        });
                      });
                } else {
                  return Text(hint);
                }
              }()),
            ),
            const Padding(padding: EdgeInsets.symmetric(horizontal: 10)),
            ElevatedButton(onPressed: getIps, child: const Text("扫描")),
          ],
        ),
        if (currentDevice != null)
          Form(
            child: Column(children: [
              TextFormField(
                controller: nameController,
                decoration: const InputDecoration(
                    labelText: "名称", hintText: "为这盏七彩祥灯起个名字"),
              ),
              TextFormField(
                controller: ledNumController,
                keyboardType: TextInputType.number,
                decoration: const InputDecoration(
                    labelText: "灯珠数量", hintText: "有几个发光LED"),
              ),
              Padding(
                  padding: const EdgeInsets.only(top: 28.0),
                  child: Row(children: <Widget>[
                    Expanded(
                      child: ElevatedButton(
                        child: const Padding(
                          padding: EdgeInsets.all(16.0),
                          child: Text("确认"),
                        ),
                        onPressed: () {
                          currentDevice!.name = nameController.text;
                          currentDevice!.ledNum =
                              int.parse(ledNumController.text);
                          log(currentDevice.toString());
                        },
                      ),
                    ),
                  ]))
            ]),
          )
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
            nameController.text = currentDevice!.name;
            ledNumController.text = currentDevice!.ledNum.toString();
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
