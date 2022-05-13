import 'dart:developer';

import 'package:controller/Effect.dart';
import 'package:flutter/material.dart';

class DrawnLine {
  final List<Offset> path;
  final Color color;
  final double width;

  DrawnLine(this.path, this.color, this.width);
}

class Sketcher extends CustomPainter {
  // final List<DrawnLine> lines;
  DrawnLine? _line;

  Sketcher(DrawnLine? line) {
    _line = line;
  }

  @override
  void paint(Canvas canvas, Size size) {
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

  late AnimationController _controller;

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
            child: Container(
              width: MediaQuery.of(context).size.width,
              // height: MediaQuery.of(context).size.height,
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
                  child:
                      CustomPaint(painter: Sketcher(line), child: Container()),
                ),
              ),
            ),
          ),
          Container(
            height: 60,
            child: AnimatedBuilder(
              animation: _controller,
              builder: (context, child) => CustomPaint(
                  painter: EffectIndicator(effect, _controller.value),
                  child: Container()),
            ),
          ),
          Center(child: const Text("A text line"))
        ],
      ),
    );
    // return const FittedBox(
    //     fit: BoxFit.fill, child: Image(image: AssetImage('assets/flame.gif')));
  }

  void onPanStart(DragStartDetails details) {
    log('User started drawing');
    final box = context.findRenderObject() as RenderBox;
    final point = box.globalToLocal(details.globalPosition);
    setState(() {
      line = DrawnLine([point], Colors.red, 20);
    });
    log(point.toString());
  }

  void onPanUpdate(DragUpdateDetails details) {
    final box = context.findRenderObject() as RenderBox;
    final point = box.globalToLocal(details.globalPosition);
    if (point.dx <= 0 || point.dy <= 0) {
      return;
    }
    setState(() {
      line!.path.add(point);
    });
    // log(point.toString());
  }

  void onPanEnd(DragEndDetails details) {
    log('User ended drawing');
  }
}
