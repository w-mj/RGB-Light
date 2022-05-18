import 'dart:developer';

import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';

class EffectList extends StatefulWidget {
  const EffectList({super.key});

  @override
  State<StatefulWidget> createState() {
    return EffectListState();
  }
}

class EffectListState extends State<EffectList> {
  List<String> effects = ["Effect1"];

  Future<void> loadEffects() async {
    final dirName = await getApplicationDocumentsDirectory();
    log("working dir $dirName");
  }

  @override
  Widget build(BuildContext context) {
    loadEffects();
    return Flex(
      direction: Axis.vertical,
      children: [
        Expanded(
          flex: 1,
          child: ListView.builder(
            scrollDirection: Axis.vertical,
            itemBuilder: (BuildContext context, int index) => ListTile(
              title: Text(effects[index]),
              onTap: () {
                log("select effect ${effects[index]}");
              },
            ),
            itemCount: effects.length,
          ),
        ),
        Flex(direction: Axis.horizontal, children: [
          Expanded(
            flex: 1,
            child: ElevatedButton(
                onPressed: () {
                  setState(() {
                    effects.add("Effect ${effects.length + 1}");
                  });
                },
                child: const Padding(
                    padding: EdgeInsets.all(8.0), child: Text("添加"))),
          )
        ])
      ],
    );
  }
}
