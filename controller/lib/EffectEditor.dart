import 'dart:async';
import 'dart:developer';
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:controller/Effect.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class DrawnLine {
  final List<Offset> path;
  final Color color;
  final double width;

  DrawnLine(this.path, this.color, this.width);
}

class Sketcher extends CustomPainter {
  // final List<DrawnLine> lines;
  DrawnLine? _line;
  ui.Image? image;

  Sketcher(DrawnLine? line, this.image) {
    _line = line;
  }

  void drawText(String text, Canvas canvas, Size size) {
    const textStyle = TextStyle(
      color: Colors.black,
      fontSize: 30,
    );
    final textSpan = TextSpan(
      text: text,
      style: textStyle,
    );
    final textPainter = TextPainter(
      text: textSpan,
      textDirection: TextDirection.ltr,
    );
    textPainter.layout(
      minWidth: 0,
      maxWidth: size.width,
    );
    final xCenter = (size.width - textPainter.width) / 2;
    final yCenter = (size.height - textPainter.height) / 2;
    final offset = Offset(xCenter, yCenter);
    textPainter.paint(canvas, offset);
  }

  @override
  void paint(Canvas canvas, Size size) {
    // if (image != null) {
    //   canvas.drawImage(image!, const ui.Offset(0, 0), Paint());
    // } else {
    //   drawText("No gif", canvas, size);
    //   return;
    // }
    if (_line == null) {
      return;
    }

    DrawnLine line = _line!;

    Paint paint = Paint()
      ..color = line.color
      ..strokeCap = StrokeCap.round
      ..strokeWidth = line.width;

    for (int j = 0; j < line.path.length - 1; ++j) {
      canvas.drawLine(line.path[j], line.path[j + 1], paint);
    }
  }

  @override
  bool shouldRepaint(Sketcher oldDelegate) {
    return true;
  }
}

class EffectIndicator extends CustomPainter {
  Effect effect;
  double value;
  int frame = 0;

  EffectIndicator(this.effect, this.value) {
    frame = (effect.fps * effect.time * value).toInt();
    if (frame == effect.fps * effect.time) {
      frame = 0;
    }
  }
  @override
  void paint(Canvas canvas, Size size) {
    if (frame >= effect.fps * effect.time) {
      frame = 0;
    }
    // log("paint $value");

    if (frame >= effect.data.length) {
      return;
    }
    double everyRectWidth = size.width / effect.ledNum;
    double everyRectHeight = size.height;
    for (int i = 0; i < effect.ledNum; i++) {
      Paint paint = Paint()
        ..color = effect.data[frame][i]
        ..strokeCap = StrokeCap.round
        ..strokeWidth = 1
        ..style = PaintingStyle.fill;
      canvas.drawRect(
          Rect.fromLTWH(i * everyRectWidth, 0, everyRectWidth, everyRectHeight),
          paint);
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) {
    return true;
  }
}

class EffectEditor extends StatefulWidget {
  const EffectEditor({Key? key}) : super(key: key);

  @override
  State<StatefulWidget> createState() {
    return EffectEditorState();
  }
}

class EffectEditorState extends State<EffectEditor>
    with SingleTickerProviderStateMixin {
  DrawnLine? line;
  Effect effect = EffectBuilder.flow();
  ui.Image? effectGif;
  late AnimationController _controller;

  EffectEditorState() {
    rootBundle.load("assets/flame.gif").then((bd) {
      Uint8List lst = Uint8List.view(bd.buffer);
      ui.instantiateImageCodec(lst).then((codec) {
        codec.getNextFrame().then((frameInfo) {
          setState(() {
            effectGif = frameInfo.image;
          });
        });
      });
    });
  }

  @override
  void initState() {
    _controller = AnimationController(
      vsync: this,
      // duration: Duration(milliseconds: 1000 ~/ effect.fps),
      duration: Duration(seconds: effect.time),
    )..repeat();
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Flex(
        direction: Axis.vertical,
        children: [
          Expanded(
            flex: 1,
            child: DefaultTabController(
              length: 2,
              child: Scaffold(
                appBar: AppBar(
                  title: TabBar(
                    tabs: ["灯效信息", "GIF"]
                        .map((e) => Tab(
                              child: Text(e),
                            ))
                        .toList(),
                  ),
                ),
                body: TabBarView(
                  children: [
                    Form(
                        child: Column(
                      children: [
                        TextFormField(
                          controller: TextEditingController(text: effect.name),
                          decoration: const InputDecoration(
                              labelText: "名称", hintText: "灯效名称"),
                        ),
                        TextFormField(
                          controller: TextEditingController(
                              text: effect.time.toString()),
                          decoration: const InputDecoration(
                              labelText: "持续时间", hintText: "灯效的持续时间"),
                        ),
                        TextFormField(
                          controller: TextEditingController(
                              text: effect.fps.toString()),
                          decoration: const InputDecoration(
                              labelText: "FPS", hintText: "帧率"),
                        ),
                        TextFormField(
                          controller: TextEditingController(
                              text: effect.ledNum.toString()),
                          decoration: const InputDecoration(
                              labelText: "灯珠数量", hintText: "灯珠数量"),
                        )
                      ],
                    )),
                    Container(
                      decoration: const BoxDecoration(
                        image: DecorationImage(
                          fit: BoxFit.fill,
                          image: AssetImage('assets/flame.gif'),
                        ),
                      ),
                      child: GestureDetector(
                        onPanStart: onPanStart,
                        onPanUpdate: onPanUpdate,
                        onPanEnd: onPanEnd,
                        child: RepaintBoundary(
                          child: CustomPaint(
                              painter: Sketcher(line, effectGif),
                              child: Container()),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.only(top: 8.0),
            child: SizedBox(
              height: 60,
              child: AnimatedBuilder(
                animation: _controller,
                builder: (context, child) => CustomPaint(
                    painter: EffectIndicator(effect, _controller.value),
                    child: Container()),
              ),
            ),
          ),
        ],
      ),
    );
    // return const FittedBox(
    //     fit: BoxFit.fill, child: Image(image: AssetImage('assets/flame.gif')));
  }

  void onPanStart(DragStartDetails details) {
    final box = context.findRenderObject() as RenderBox;
    final point = box.globalToLocal(details.globalPosition);
    log('User started drawing $point, ${details.localPosition}');
    setState(() {
      line = DrawnLine([details.localPosition], Colors.red, 20);
    });
    // log(point.toString());
  }

  void onPanUpdate(DragUpdateDetails details) {
    final box = context.findRenderObject() as RenderBox;
    final point = box.globalToLocal(details.globalPosition);
    if (point.dx <= 0 || point.dy <= 0) {
      return;
    }
    setState(() {
      line!.path.add(details.localPosition);
    });
    // log(point.toString());
  }

  void onPanEnd(DragEndDetails details) {
    log('User ended drawing');
  }
}
