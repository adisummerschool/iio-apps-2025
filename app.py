from queue import Queue
from threading import Thread, Event
from pynput import keyboard
from pynput.keyboard import Controller
import time
import iio
import math

import tkinter as tk 

URI = "ip:10.76.84.118"
DEVICE = "ad5592r_s"
TIMEOUT = 0.4

def create_gui(queue):
        def update_sq_color(canvas, sq, value):
                    #use canvas.itemconfing
                    #color = f'#{intesity:02x}{intensity:02x}ff'
                    if value == 1:
                        canvas.itemconfig(sq, fill= 'red')
                    elif value > 0.7 and value < 1:
                        canvas.itemconfig(sq, outline = 'red', width = 30)                    
                    elif value > 0.5 and value < 0.7:
                        canvas.itemconfig(sq, outline = 'red', width = 20)
                    elif value > 0.2 and value < 0.5:
                        canvas.itemconfig(sq, outline = 'red', width = 10)
                    elif value < 0.2:
                        canvas.itemconfig(sq, fill='brown', outline = 'brown', width = 10)
        def update_gui():
              
              mov = queue.get()
              print(mov)
              
              update_sq_color(canvas, squares['left'], mov['left'])
              update_sq_color(canvas, squares['right'], mov['right'])
              update_sq_color(canvas, squares['front'], mov['front'])
              update_sq_color(canvas, squares['back'], mov['back'])

              root.after(int(TIMEOUT * 1000), update_gui)

        root = tk.Tk()
        root.title("key simulator")

        canvas = tk.Canvas(root, width=400, height=400)
        canvas.pack()
        square_size = 20

        canvas.create_rectangle(50, 150, 50 + square_size, 150+ square_size, fill='brown')

        square_size = 100
        squares = {
            'left':canvas.create_rectangle(50, 150, 50 + square_size, 150+ square_size, fill='red'),
            'right':canvas.create_rectangle(250, 150, 250 + square_size, 150+ square_size, fill='red'),            
            'front':canvas.create_rectangle(150, 50, 150 + square_size, 50+ square_size, fill='red'),
            'back':canvas.create_rectangle(150, 250, 150 + square_size, 250+ square_size, fill='red')
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
            'x' : {"-" : int, '+': int},
            'y' : {"-" : int, '+': int},
            'z' : {"-" : int, '+': int},
    }

    #[int, int, int, int, int, int]
    for i, channel_name in enumerate(channel_names):
        channel = device.find_channel(channel_name)
        if channel is None:
            raise ValueError("Channel not found")
        attr = channel.attrs['raw'].value
        #populati axis_data
        if not i % 2:
               axis_data[axes[i//2]]["+"] = int(attr)
        else:
               axis_data[axes[i//2]]["-"] = int(attr)
        
        
    return axis_data

def get_roll_pitch(data):
       x_accel = data['x']["+"] - data['x']["-"]
       y_accel = data['y']["+"] - data['y']["-"]
       z_accel = data['z']["+"] - data['z']["-"]


       pitch = math.atan2(y_accel, z_accel) * 180 / math.pi
       roll = math.atan2(-x_accel, math.sqrt(y_accel ** 2 + z_accel**2 )) * 180 / math.pi

       return roll, pitch

def get_movement(roll, pitch):
    movements = {
          'left' : 0,
          'right' : 0,
          'front' : 0,
          'back' : 0
    }

    limit = 45

    if roll < 0:
        movements['left'] = min (1 , abs(roll) / limit) 
    else :
        movements['right'] = min (1 , abs(roll) / limit)

    if pitch < 0:
        movements['front'] = min (1 , abs(pitch)/ limit)
    else :
        movements['back'] = min (1 , abs(pitch) / limit)

    return movements

def simulate_key(key, on_time):
    kb_controller = Controller()

    if on_time > 0:
        kb_controller.press(key)
        time.sleep(on_time)
        kb_controller.release(key)

def start_iio(device : iio.Device):
    start = time.time()
    #init_movements = {'left': 0, 'right': 0, 'front': 0, 'back': 0}
    data = get_data(device)
    roll, pitch = get_roll_pitch(data)
    movement = get_movement(roll, pitch)

    end = time.time() - start

    for direction, key in [('front', 'w'), ('back', 's'), ('left', 'a'), ('right', 'd')]:
        value = movement[direction]
        if value > 0.2 and value < 1:
            on_time = value * TIMEOUT
            key_thread = Thread(target=simulate_key, args=(key, on_time))
            key_thread.daemon = True
            key_thread.start()

    time.sleep(TIMEOUT - end)

    return movement
    

def simulate_movement(queue: Queue):
    device = init_device()

    while not exit_event.is_set():
        if start_event.is_set():
            queue.put(start_iio(device))

            #print("Running")
            #movements = queue.get()
            #movements['left'] = movements['left'] + 1
            #queue.put(movements)
            #time.sleep(1)
        start_event.wait()

def on_keypress(key):
        if key == keyboard.Key.ctrl_l and not start_event.is_set():
                start_event.set()
                print("Start")
        elif key == keyboard.Key.ctrl_l and start_event.is_set():
                start_event.clear()
                print("Pause")
        elif key == keyboard.Key.esc:
                exit_event.set()
                print("Esti rege")
                return False
        return True



if __name__ == '__main__':

        start_event = Event()
        exit_event = Event()

        init_movements = {'left': 0, 'right': 0, 'front': 0, 'back': 0}
        movement_q = Queue()
        movement_q.put(init_movements)

        gui_thread = Thread(target=create_gui, args=(movement_q, ))
        gui_thread.daemon = True
        gui_thread.start()

        iio_thread = Thread(target=simulate_movement, args=(movement_q, ))
        iio_thread.daemon = True
        iio_thread.start()

        print("Press Left CTRL to start/stop application. Press ESC to exit")

        with keyboard.Listener(on_press=on_keypress) as listener:
                listener.join() #asteapta keyrpressul


        #gui_thread.join()