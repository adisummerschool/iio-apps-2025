import math
from queue import Queue
from threading import Thread, Event
from pynput import keyboard
import time
import iio
from pynput.keyboard import Controller
import tkinter as tk

URI = "ip:10.76.84.230"
DEVICE = "iio-adc-ad5592"
TIMEOUT = 0.3

def create_gui(queue: Queue):
    def update_sq_color(canvas, square, value):
        intensity = int(255 * (1 - min(max(value, 0), 1)))
        color = f'#{intensity:02x}{intensity:02x}{intensity:02x}'
        canvas.itemconfig(square, fill=color)

    def update_gui():
        movement = queue.get()
        update_sq_color(canvas, squares['left'], movement['left'])
        update_sq_color(canvas, squares['right'], movement['right'])
        update_sq_color(canvas, squares['front'], movement['front'])
        update_sq_color(canvas, squares['back'], movement['back'])
        root.after(int(TIMEOUT * 1000), update_gui)

    root = tk.Tk()
    root.title("Movement Simulator")
    canvas = tk.Canvas(root, width=400, height=400)
    canvas.pack()
    square_size = 100
    squares = {
        "left": canvas.create_rectangle(50, 150, 50 + square_size, 150 + square_size, fill="blue"),
        "right": canvas.create_rectangle(250, 150, 250 + square_size, 150 + square_size, fill="green"),
        "front": canvas.create_rectangle(150, 50, 150 + square_size, 50 + square_size, fill="yellow"),
        "back": canvas.create_rectangle(150, 250, 150 + square_size, 250 + square_size, fill="red"),
    }
    update_gui()
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

    # Calculate left/right
    if roll < 0:
        left = min(1, -roll / limit)
        right = 0
    else:
        right = min(1, roll / limit)
        left = 0

    # Calculate front/back
    if pitch < 0:
        front = min(1, -pitch / limit)
        back = 0
    else:
        back = min(1, pitch / limit)
        front = 0

    # Only allow one of left/right and one of front/back
    if left > right:
        movements['left'] = left
    elif right > 0:
        movements['right'] = right

    if front > back:
        movements['front'] = front
    elif back > 0:
        movements['back'] = back

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

    return movement


def simulate_movement(queue):
    device = init_device()

    while not exit_event.is_set():
        if start_event.is_set():
            time.sleep(1)
            queue.put(start_iio(device))
        start_event.wait()

def on_keypress(key):
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

