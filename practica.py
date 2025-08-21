import math
from queue import Queue
from threading import Thread, Event
from pynput import keyboard
import time
import iio
from pynput.keyboard import Controller

URI = "ip:10.76.84.238"
DEVICE = "iio-adc-ad5592"
TIMEOUT = 0.3

import tkinter as tk
def create_gui(queue):
    root = tk.Tk()
    root.title("Analog_Practica")

    # Get screen resolution
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()

    # Window is quarter of screen size
    win_width = screen_width // 2
    win_height = screen_height // 2
    root.geometry(f"{win_width}x{win_height}")

    canvas = tk.Canvas(root, width=win_width, height=win_height, bg="black")
    canvas.pack(fill="both", expand=True)

    # Square size = half of canvas width/height
    sq_w = win_width // 2
    sq_h = win_height // 2

    # Front (top-center)
    front = canvas.create_rectangle(
        sq_w//2, 0, sq_w + sq_w//2, sq_h,
        fill="gray", tags="front"
    )

    # Back (bottom-center)
    back = canvas.create_rectangle(
        sq_w//2, sq_h, sq_w + sq_w//2, 2*sq_h,
        fill="gray", tags="back"
    )

    # Left (middle-left)
    left = canvas.create_rectangle(
        0, sq_h//2, sq_w, sq_h + sq_h//2,
        fill="gray", tags="left"
    )

    # Right (middle-right)
    right = canvas.create_rectangle(
        sq_w, sq_h//2, 2*sq_w, sq_h + sq_h//2,
        fill="gray", tags="right"
    )

    squares = {
        "front": front,
        "back": back,
        "left": left,
        "right": right
    }

    def value_to_color(value):
        """Map 0–1 to a grayscale intensity."""
        intensity = int(value * 255)
        return f"#{intensity:02x}0000"   # red-scale (00 to FF)

    def update_colors():
        try:
            movements = queue.get_nowait()
            for direction, val in movements.items():
                color = value_to_color(val)
                canvas.itemconfig(squares[direction], fill=color)
        except:
            pass
        root.after(100, update_colors)

    update_colors()
    root.mainloop()

def init_device():
    ctx = iio.Context(URI)
    device = ctx.find_device(DEVICE)

    if device is None:
        return ValueError("No device")
    return device


def get_data(device):
    channel_names = [f'voltage{i}' for i in range(6)]
    axes = ['x', 'y', 'z']
    axis_data = {
        'x': {'-': int, '+': int},
        'y': {'-': int, '+': int},
        'z': {'-': int, '+': int},
    }

    for i, channel_name in enumerate(channel_names):
        channel = device.find_channel(channel_name)
        if channel is None:
            return ValueError(f"Channel {i} not found")

        attr = channel.attrs['raw'].value
        axis_data[axes[i // 2]]['+' if i % 2 == 0 else '-'] = int(attr)

    return axis_data


def get_roll_pitch(data):
    x_accel = data['x']['+'] - data['x']['-']
    y_accel = data['y']['+'] - data['y']['-']
    z_accel = data['z']['+'] - data['z']['-']

    pitch = math.atan2(y_accel, z_accel) * 180 / math.pi
    roll = math.atan2(-x_accel, math.sqrt(y_accel ** 2 + z_accel ** 2)) * 180 / math.pi
    return roll, pitch


def get_movement(roll, pitch):
    movements = {
        "left": 0,
        "right": 0,
        "front": 0,
        "back": 0
    }

    limit = 45

    if roll < 0:
        movements['left'] = min(1, -roll / limit)
    else:
        movements['right'] = min(1, roll / limit)

    if pitch < 0:
        movements['front'] = min(1, -pitch / limit)
    else:
        movements['back'] = min(1, pitch / limit)

    return movements


def simulate_key(key, on_time):
    kb_controller = Controller()

    if on_time > 0:
        kb_controller.press(key)
        time.sleep(on_time)
        kb_controller.release(key)


def start_iio(device):
    start = time.time()

    data = get_data(device)
    roll, pitch = get_roll_pitch(data)
    movement = get_movement(roll, pitch)

    timer = time.time() - start

    for direction, key in [("front", 'w'), ("back", 's'), ("left", 'a'), ("right", 'd')]:
        value = movement[direction]
        if value > 0.2:
            on_time = TIMEOUT * value
            key_thread = Thread(target=simulate_key, args=(key, on_time))
            key_thread.daemon = True
            key_thread.start()

    time.sleep(TIMEOUT - timer)

    return init_movements


def simulate_movement(queue):
    device = init_device()

    while not exit_event.is_set():
        if start_event.is_set():
            time.sleep(1)
            queue.put(start_iio(device))
        start_event.wait()


#

def on_keypress(key):
    # keyboard.Key.ctrl_l
    if key == keyboard.Key.ctrl_l:
        if start_event.is_set():
            start_event.clear()
            print('pause')
        else:
            start_event.set()
            print('start')
    if key == keyboard.Key.esc:
        print('end')
        exit_event.set()
        return False

    return True

if __name__ == '__main__':
    start_event = Event()
    exit_event = Event()

    init_movements = {"left": 0, "right": 0, "front": 0, "back": 0}
    movement_q = Queue()
    movement_q.put(init_movements)

    gui_thread = Thread(target=create_gui, args=(movement_q,))
    gui_thread.daemon = True
    gui_thread.start()

    iio_thread = Thread(target=simulate_movement, args=(movement_q,))
    iio_thread.daemon = True
    iio_thread.start()

    print("Press Left Ctrl to start/stop application. Press Esc to exit")

    with keyboard.Listener(on_press=on_keypress) as listener:
        listener.join()

