import json


def rgb(r, g, b):
    return (r << 16) + (g << 8) + b


def breath():
    effect = {
        'total_frames': 240,
        'ms_per_frame': 10,
        'name': 'breath',
        'frames': []
    }

    for t in range(effect['total_frames']):
        frame = [rgb(t % 256, t % 256, t % 256) for i in range(60)]
        effect['frames'].append(frame)

    json.dump(effect, open(f"effect_{effect['name']}.json", "w"))


def flow():
    effect = {
        'total_frames': 240,
        'ms_per_frame': 20,
        'name': 'flow',
        'frames': []
    }

    for t in range(effect['total_frames']):
        frame = [0] * 60
        for i in range(60):
            if (i + t) % 3 == 0:
                frame[i] = rgb(255, 0, 0)
            elif (i + t) % 3 == 1:
                frame[i] = rgb(0, 255, 0)
            else:
                frame[i] = rgb(0, 0, 255)
        effect['frames'].append(frame)

    json.dump(effect, open(f"effect_{effect['name']}.json", "w"))


if __name__ == '__main__':
    breath()
    flow()
