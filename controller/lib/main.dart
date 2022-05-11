import 'package:flutter/material.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Fancy Light Controller',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: Scaffold(body: MainPage()),
    );
  }
}

class MainPage extends StatefulWidget {
  @override
  State<StatefulWidget> createState() {
    return MainPageState();
  }
}

class MainPageState extends State<MainPage> {
  List<DropdownMenuItem<String>> sortItems = [];
  String deviceListValue = '未选择';
  MainPageState() : super() {
    sortItems.add(DropdownMenuItem(value: '未选择', child: Text('未选择')));
    sortItems.add(DropdownMenuItem(value: '设备1', child: Text('设备1')));
    sortItems.add(DropdownMenuItem(value: '设备2', child: Text('设备2')));
  }

  @override
  Widget build(BuildContext context) {
    return Flex(
      direction: Axis.horizontal,
      children: [
        Expanded(
          flex: 1,
          child: Column(
            children: [
              Flex(
                direction: Axis.horizontal,
                children: [
                  const Padding(padding: EdgeInsets.symmetric(horizontal: 20)),
                  Expanded(
                    flex: 1,
                    child: DropdownButton(
                      value: deviceListValue,
                      items: sortItems,
                      onChanged: (value) {
                        setState(() {
                          deviceListValue = value.toString();
                        });
                      },
                    ),
                  ),
                  const Padding(padding: EdgeInsets.symmetric(horizontal: 10)),
                  ElevatedButton(onPressed: () {}, child: Text("搜索")),
                  const Padding(padding: EdgeInsets.symmetric(horizontal: 20))
                ],
              ),
              Text("slect 2"),
              Text("slect 4"),
            ],
          ),
        ),
        Expanded(
          flex: 2,
          child: const Image(image: AssetImage('assets/flame.gif')),
        )
      ],
    );
  }
}
