import 'dart:ui';

Color hsvToRgb(double H, double S, double V) {
  int R, G, B;

  H /= 360;
  S /= 100;
  V /= 100;

  if (S == 0) {
    R = (V * 255).toInt();
    G = (V * 255).toInt();
    B = (V * 255).toInt();
  } else {
    double var_h = H * 6;
    if (var_h == 6) var_h = 0; // H must be < 1
    int var_i = var_h.floor(); // Or ... var_i =
    // floor( var_h )
    double var_1 = V * (1 - S);
    double var_2 = V * (1 - S * (var_h - var_i));
    double var_3 = V * (1 - S * (1 - (var_h - var_i)));

    double var_r;
    double var_g;
    double var_b;
    if (var_i == 0) {
      var_r = V;
      var_g = var_3;
      var_b = var_1;
    } else if (var_i == 1) {
      var_r = var_2;
      var_g = V;
      var_b = var_1;
    } else if (var_i == 2) {
      var_r = var_1;
      var_g = V;
      var_b = var_3;
    } else if (var_i == 3) {
      var_r = var_1;
      var_g = var_2;
      var_b = V;
    } else if (var_i == 4) {
      var_r = var_3;
      var_g = var_1;
      var_b = V;
    } else {
      var_r = V;
      var_g = var_1;
      var_b = var_2;
    }

    R = (var_r * 255).toInt(); // RGB results from 0 to 255
    G = (var_g * 255).toInt();
    B = (var_b * 255).toInt();
  }
  return Color.fromARGB(255, R, G, B);
}

class Effect {
  int time; // 持续时间
  int fps;
  int ledNum;

  List<List<Color>> data = [];

  Effect(this.time, this.fps, this.ledNum);
}

class EffectBuilder {
  static Effect flow() {
    Effect effect = Effect(10, 12, 60);
    for (int i = 0; i < effect.time * effect.fps; i++) {
      List<Color> tmp =
          List.filled(effect.ledNum, const Color.fromARGB(0, 255, 255, 255));
      for (int j = 0; j < effect.ledNum; j++) {
        tmp[j] = hsvToRgb((j + i * 30) % 360, 100, 100);
      }
      effect.data.add(tmp);
    }
    return effect;
  }
}
