import 'package:controller/DeviceScan.dart';
import 'package:controller/EffectEditor.dart';
import 'package:controller/EffectList.dart';
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
  const MainPage({Key? key}) : super(key: key);

  @override
  State<StatefulWidget> createState() {
    return MainPageState();
  }
}

class MainPageState extends State<MainPage> {
  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Flex(
        direction: Axis.horizontal,
        children: [
          Expanded(
            flex: 1,
            child: Flex(
              direction: Axis.vertical,
              children: const [
                DeviceScan(),
                Expanded(flex: 1, child: EffectList())
              ],
            ),
          ),
          const Expanded(
            flex: 2,
            child: EffectEditor(),
          )
        ],
      ),
    );
  }
}
