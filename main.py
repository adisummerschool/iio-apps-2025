from queue import Queue
from threading import Thread, Event
import time
import math
import tkinter as tk # pip install tk

import iio

from pynput import keyboard #pip install pynput
from pynput.keyboard import Controller

URI = "ip:10.76.84.213"
DEVICE = "ad5592r_s"
TIMEOUT = 0.3

def on_keypress(key):
        # keyboard.Key.ctrl_l
        # start_event.is_set() returns bool
        # start_event.clear() unsets event
        # start_event.set() sets event
        if key == keyboard.Key.ctrl_l:
                if start_event.is_set():
                        start_event.clear()
                        print("Clear")
                else:
                        start_event.set()
                        print("Set")
        if key == keyboard.Key.esc:
                exit_event.set()
                print("Exit")
                return False
        return True

def color_from_value(value, base):
        r = int(base[0] * value + 255 * (1 - value))  
        g = int(base[1] * value + 255 * (1 - value))
        b = int(base[2] * value + 255 * (1 - value))
        return f"#{r:02x}{g:02x}{b:02x}"


def create_gui(queue: Queue):

        def update_sq_color(canvas, sq, value, base):
                val = float(value)
                new_color = color_from_value(val, base)
                canvas.itemconfig(sq, fill = new_color)
                pass
        
        def update_gui():
                movement = queue.get()

                update_sq_color(canvas, squares['left'], movement['front'], (255, 0, 0))
                update_sq_color(canvas, squares['right'], movement['back'], (0, 255, 0))
                update_sq_color(canvas, squares['front'], movement['left'], (0, 0, 255))
                update_sq_color(canvas, squares['back'], movement['right'], (255, 255, 0))

                root.after(int(TIMEOUT * 1000), update_gui)

        
        root = tk.Tk()
        root.title("Key Simulator")
        canvas = tk.Canvas(root, width = 400, height = 400)
        canvas.pack()

        square_size = 100
        squares = {
                'left': canvas.create_rectangle(50, 150, 50 + square_size, 150 + square_size, fill = 'white'),
                'right': canvas.create_rectangle(250, 150, 250 + square_size, 150 + square_size, fill = 'white'),
                'front': canvas.create_rectangle(150, 50, 150 + square_size, 50 + square_size, fill = 'white'),
                'back': canvas.create_rectangle(150, 250, 150 + square_size, 250 + square_size, fill = 'white'),
        }

        update_gui()
        root.mainloop()


                
def init_device():
        ctx = iio.Context(URI)
        device = ctx.find_device(DEVICE)

        if device is None:
                raise ValueError("No device")
        
        return device



def get_data(device: iio.Device):
        channel_names = [f'voltage{i}' for i in range(6)]
        axes = ['x', 'y', 'z']
        axis_data = {
                'x':{'-': int, '+': int},
                'y':{'-': int, '+': int},
                'z':{'-': int, '+': int},
        }

        for i, channel_name in enumerate(channel_names):
                channel = device.find_channel(channel_name)
                if channel is None:
                        raise ValueError("Channel not found")
                
                attr = channel.attrs['raw'].value
                # populate axis_data
                # axis_data ['x']['-'] = attr
                if i % 2 == 0:
                        axis_data[axes[i//2]]['+'] = attr
                else:
                        axis_data[axes[i//2]]['-'] = attr
        # print(axis_data['x']['+'] + " " + axis_data['x']['-'] + "     " +
        #        axis_data['y']['+'] + " " + axis_data['y']['-'] + "     " + 
        #        axis_data['z']['+'] + " " + axis_data['z']['-'])
        return axis_data


def get_roll_pitch(data):
        x_accel = int(data['x']['+']) - int(data['x']['-'])
        y_accel = int(data['y']['+']) - int(data['y']['-'])
        z_accel = int(data['z']['+']) - int(data['z']['-'])

        roll = math.atan2(y_accel, z_accel) * 180 / math.pi
        pitch = math.atan2(-x_accel, math.sqrt(y_accel ** 2 + z_accel **2)) * 180 / math.pi

        return roll, pitch

def get_movement(roll, pitch):
        movements = {
                'left': 0,
                'right': 0,
                'front': 0,
                'back': 0
        }

        if roll < 0:
                movements['left'] = min(1, abs(roll) / 45)
        else:
                movements['right'] = min(1, abs(roll) / 45)

        if pitch < 0:
                movements['front'] = min(1, abs(pitch) / 45)
        else:
                movements['back'] = min(1, abs(pitch) / 45)
        
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
        #print (roll, pitch)
        movement = get_movement(roll, pitch)

        timer = time.time() - start

        for direction, key in [('front', 'a'), ('back', 'd'), ('left', 'w'), ('right', 's')]:
                value = movement[direction]
                if value > 0.2:
                        on_time = TIMEOUT * value
                        key_thread = Thread(target=simulate_key, args=(key, on_time))
                        key_thread.daemon = True
                        key_thread.start()

        time.sleep(TIMEOUT - timer)
        # init_movements = {'left': 0, 'right': 0, 'front': 0, 'back': 0}
        return movement

# 

def simulate_movement(queue: Queue):
        device = init_device()

        while not exit_event.is_set():
                if start_event.is_set():
                        queue.put(start_iio(device))
                        #print("running")
                        # queue.get()
                        # increment movement left value
                        # mvm = queue.get()
                        # mvm['left'] += 1
                        # queue.put(mvm)
                        # time.sleep(1)
                start_event.wait()
                

if __name__ == '__main__':
        start_event = Event()
        exit_event = Event()

        init_movements = {'left': 0, 'right': 0, 'front': 0, 'back': 0}
        movement_q = Queue()
        movement_q.put(init_movements)

        gui_thread = Thread(target=create_gui, args=(movement_q,))
        gui_thread.daemon = True
        gui_thread.start()

        iio_thread = Thread(target=simulate_movement, args=(movement_q,))
        iio_thread.daemon = True
        iio_thread.start()

        print("Press Left Ctrl to start/stop application. Press Esc to exit")

        with keyboard.Listener(on_press = on_keypress) as listener:
                listener.join()

        #gui_thread.join()