from queue import Queue
from threading import Thread, Event
import time
import iio
import math
from pynput import keyboard # pip install pynput
from pynput.keyboard import Controller

URI = "ip:10.76.84.222"
DEVICE = "iio-ad5592"
TIMEOUT = 0.3

def create_gui(queue: Queue):
        # print("gui")
        while not exit_event.is_set():
                if start_event.is_set():
                        movement = queue.get()
                        # print(movement)
                        queue.put(movement)
                        # time.sleep(1)
                start_event.wait()

def init_device():
        ctx = iio.Context(URI)
        device = ctx.find_device(DEVICE)

        if device is None:
                raise ValueError("No device")
        
        return device

def get_data(device: iio.Device):
        channel_names = [f'voltage{i}' for i in range(6)]
        axis = ['x', 'y', 'z']
        axis_data = {
                'x': {"-": int, '+': int},
                'y': {"-": int, '+': int},
                'z': {"-": int, '+': int},
        }

        for i, channel_name in enumerate(channel_names):
                channel = device.find_channel(channel_name)
                if channel is None:
                        raise ValueError("Channel not found")
                
                attr = channel.attrs['raw'].value

                if not i%2:
                       axis_data[axis[i//2]]['+'] = int(attr)
                elif i%2 == 1: 
                        axis_data[axis[i//2]]['-'] = int(attr)

        # print(axis_data)
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
                'left': 0,
                'right': 0,
                'front': 0,
                'back': 0
        }

        if roll < 0:
                movements['left'] = min(abs(roll)/45, 1) 
        else:
                movements['right'] = min(abs(roll)/45, 1)

        if pitch < 0:
                movements['front'] = min(abs(pitch)/45, 1)
        else:
                movements['back'] = min(abs(pitch)/45, 1)

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
        movement = get_movement(roll, pitch)
        # print("Roll: ", roll, "Pitch: ", pitch)
        # print(movement)
        # time.sleep(0.5)

        timer = time.time() - start

        for direction, key in [('front','w'),('back', 's'),('left', 'a'), ('right','d')]:
                value = movement[direction]
                if value > 0.2:
                      on_time = TIMEOUT * value
                      key_thread = Thread(target=simulate_key, args=(key, on_time))
                      key_thread.daemon = True
                      key_thread.start()  

        time.sleep(max(0,TIMEOUT - timer))

        return init_movements

def simulate_movement(queue: Queue):
        
        device = init_device()
        
        while not exit_event.is_set():
                if start_event.is_set():
                        queue.put(start_iio(device))
                        # print("running")
                        # movement = queue.get()
                        # movement["left"] += 1
                        # queue.put(movement)
                        # time.sleep(1)
                start_event.wait()        

def on_keypress(key):
        # keyboard.Key.ctrl_l
        # start_event.is_set()
        #start_event.clear()
        #start_event.set()
        if key == keyboard.Key.ctrl_l and start_event.is_set():
                start_event.clear()
                print("pause")
                return True
        
        if key == keyboard.Key.ctrl_l and not start_event.is_set():
                start_event.set()
                print("start")
                return True
        
        if key == keyboard.Key.esc:
                exit_event.set()
                print("exit")
                return False     
        


if __name__ == '__main__':
        start_event = Event()
        exit_event = Event()

        init_movements = {'left':0, 'right': 0, 'front': 0, 'back': 0}
        movement_q = Queue()
        movement_q.put(init_movements)

        gui_thread = Thread(target=create_gui, args=(movement_q, ))
        gui_thread.daemon = True
        gui_thread.start()

        iio_thread = Thread(target=simulate_movement, args=(movement_q, ))
        iio_thread.daemon = True
        iio_thread.start()

        print("Press Left Ctrl to start/stop application. Press Esc to exit")

        with keyboard.Listener(on_press=on_keypress) as listener:
                listener.join()

        