from queue import Queue
from threading import Thread, Event
from pynput import keyboard
from pynput.keyboard import Controller
import time
import math
import iio
import tkinter as tk

URI = "ip:10.76.84.219"
DEVICE = 'iio-ad5592r-s'

TIMEOUT = 0.2


def create_gui(queue):

    def _from_rgb(rgb):
        return "#%02x%02x%02x" % rgb   

    def update_sq_color(canvas, sq, value):
         value = 1 - value
         canvas.itemconfig(sq, fill = _from_rgb( (255, int(255*value), int(255*value)) ) )

    def update_gui():
        movement = queue.get()

        update_sq_color(canvas, squares['left'],  movement['left'])
        update_sq_color(canvas, squares['right'], movement['right'])
        update_sq_color(canvas, squares['front'], movement['front'])
        update_sq_color(canvas, squares['back'],  movement['back'])
        
        # print(movement)
        root.after(int(TIMEOUT * 1000), update_gui)


    root = tk.Tk()
    root.title('key simulator')

    canvas = tk.Canvas(root, width=400, height=400)
    canvas.pack()

    square_size = 100

    squares = {
         'left'  : canvas.create_rectangle(50, 150, 50 + square_size, 150 + square_size, fill= 'white'),
         'right' : canvas.create_rectangle(250, 150, 250 + square_size, 150 + square_size, fill= 'white'),
         'front' : canvas.create_rectangle(150, 50, 150 + square_size, 50 + square_size, fill= 'white'),
         'back'  : canvas.create_rectangle(150, 250, 150 + square_size, 250 + square_size, fill= 'white'),
    }
    update_gui()
    root.mainloop()     

def init_device():
    ctx = iio.Context(URI)
    if ctx is None:
            raise ValueError("No context")
    device = ctx.find_device(DEVICE)

    if device is None:
            raise ValueError("No device")

    return device

def get_data(device: iio.Device):
    channel_names = [f'voltage{i}' for i in range(6)]
    axes = ['x', 'y', 'z']
    signs = ['+', '-']
    axis_data = {
         'x': {'-' : int, '+' : int},
         'y': {'-' : int, '+' : int},
         'z': {'-' : int, '+' : int},
    }
    
    for i, channel_name in enumerate(channel_names):
        channel = device.find_channel(channel_name)
        if channel is None:
                raise ValueError("Chanel not found")
        attr = channel.attrs['raw'].value
        axis_data[axes[i//2]][signs[i%2]] = int(attr)
    
    # print(axis_data)
    return axis_data

def get_roll_pitch(data):
    x_accel = data['x']['+'] - data['x']['-']
    y_accel = data['y']['+'] - data['y']['-']
    z_accel = data['z']['+'] - data['z']['-']

    roll = math.atan2(y_accel, z_accel) * 180 / math.pi
    pitch = math.atan2(-x_accel, math.sqrt(y_accel ** 2 + z_accel ** 2 )) * 180 / math.pi

    return roll, pitch

def get_movement(roll, pitch):
     movements = {
          'left' : 0,
          'right' : 0,
          'front' : 0,
          'back' : 0
     }

     if roll < 0:
        movements['left'] = min(1, abs(roll) / 45.0)
     else:
        movements['right'] = min(1, abs(roll) / 45.0)

     if pitch < 0:
        movements['back'] = min(1, abs(pitch) / 45.0)
     else:
        movements['front'] = min(1, abs(pitch) / 45.0)

     return movements

def simulate_key(key, on_time):
     kb_controller = Controller()
     if on_time > 0:
        kb_controller.press(key)
        time.sleep(on_time)
        kb_controller.release(key)

def start_iio(device: iio.Device):

    start = time.time()

    data = get_data(device)
    roll, pitch = get_roll_pitch(data)
    # print(roll, pitch)

    movement = get_movement(roll, pitch)
    #print(movement)

    timer = time.time() - start

    for direction, key in [('front', 'w'), ('back', 's'), ('left', 'a'), ('right', 'd')]:
        value = movement[direction]
        if value > 0.2:
             on_time = TIMEOUT * value
             key_thread = Thread(target = simulate_key, args=(key, on_time))
             key_thread.daemon = True
             key_thread.start()

    time.sleep(TIMEOUT- timer)
    #
    return movement

def simulate_movement(queue: Queue):
    device = init_device()

    while not exit_event.is_set():
            if start_event.is_set():
                 queue.put(start_iio(device))
                    # print("running")
                    # mvm = queue.get()
                    # mvm['left'] += 1
                    # queue.put(mvm)
                    # time.sleep(1)
            start_event.wait()

def on_keypress(key):
    # keyboard.Key.ctrl_l
    if key == keyboard.Key.ctrl_l:
        if start_event.is_set():
            start_event.clear()
            print('pause')
        else:
            start_event.set()
            print('start')
        return True
        
    elif key == keyboard.Key.esc:
        exit_event.set
        print('exit')
        return False

if __name__ == '__main__':

    start_event = Event()
    exit_event = Event()


    init_movements = {'left' : 0, 'right' : 0, 'front' : 0, 'back' : 0}
    movement_q = Queue()
    movement_q.put(init_movements)

    gui_thread  = Thread(target=create_gui, args=(movement_q,))
    gui_thread.daemon = True
    gui_thread.start()

    iio_thread  = Thread(target=simulate_movement, args=(movement_q,))
    iio_thread.daemon = True
    iio_thread.start()

    print("Press Left ctrl to start/stop application. Press Esc to exit")

    with keyboard.Listener(on_press=on_keypress) as listener:
        listener.join()

    # gui_thread.join()

