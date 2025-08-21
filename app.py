from queue import Queue
from threading import Thread, Event
import time
import iio 
import math
import tkinter as tk
from pynput import keyboard
from pynput.keyboard import Controller
URI ="ip:10.76.84.217"
DEVICE ="ad5592r_s"
TIMEOUT = 0.3

def create_gui(queue: Queue):
        def update_sq_color(canvas, sq, value):
    # value is between 0 and 1
                red = int(255 * value)
                green = int(255 * (1 - value))
                blue = 0   # keep blue at 0 to avoid grey
    
                color = f"#{red:02x}{green:02x}{blue:02x}"
                canvas.itemconfig(sq, fill=color)

             
#
        def update_gui():
                movement = queue.get()
                print(movement)
        
                update_sq_color(canvas,squares['left'],movement['left'])
                update_sq_color(canvas,squares['right'],movement['right'])
                update_sq_color(canvas,squares['front'],movement['front'])
                update_sq_color(canvas,squares['back'],movement['back'])
                root.after(int(TIMEOUT*1000),update_gui)
                
        #print("gui") 
        root = tk.Tk()
        root.title("Key simulator")
        canvas = tk.Canvas(root, width=400, height=600)
        canvas.pack()
        square_size = 100
        squares = {
                'left':canvas.create_rectangle(50, 150, 50 + square_size,150 + square_size,fill ='white'),
                'right':canvas.create_rectangle(250, 150, 250 + square_size,150 + square_size,fill ='white'),
                'front':canvas.create_rectangle(150, 50, 150 + square_size,50 + square_size,fill ='white'),
                'back':canvas.create_rectangle(150, 250, 150 + square_size,250 + square_size,fill ='white'),
        }
        update_gui()

        root.mainloop()
        #while true and print movement left value
#       while not exit_event.is_set():
#                 if start_event.is_set():
        #                 mvm= queue.get()
        #       #  mvm['left'] +=1   
        #                 print(mvm)
        #                 queue.put(mvm)
        #                 time.sleep(1)
                # start_event.wait()   
             

def init_device():   
        ctx =iio.Context(URI) 
        device = ctx.find_device(DEVICE)

        if device is None:
              raise ValueError("No device")
        return device 

def get_data(device: iio.Device):
        channel_names = [f'voltage{i}' for i in range(6)]
        axes = ['x','y', 'z']
        axis_data = {                   #dictionar
                'x':{"-": int, '+': int},
                'y':{"-": int, '+': int},
                'z':{"-": int, '+': int},
        }
        for i, channel_name in enumerate(channel_names):  
                channel = device.find_channel(channel_name)
                if channel is None:
                        raise ValueError("Channel not found")
                attr = channel.attrs['raw'].value   
                #populati axis_data
                if i % 2 == 0:
                        axis_data[axes[i // 2]]['+'] = int(attr)
                else:
                        axis_data[axes[i // 2]]['-'] = int(attr)    

                # print (axis_data)

        return axis_data
        # 

def get_roll_pitch(data):
        x_accel = float(data['x']['+']) - float(data['x']['-'])
        y_accel = float(data['y']['+']) - float(data['y']['-'])
        z_accel = float(data['z']['+']) - float(data['z']['-'])

       
        pitch = math.atan2(y_accel, z_accel)* 180 / math.pi              #pitch=stanga/dreapta
        roll= math.atan2(-x_accel, math.sqrt(y_accel ** 2 + z_accel **2)) *180/math.pi         #roll=fata/space 

        return roll,pitch  

def get_movement(roll,pitch):
        movements ={
                'left':0,
                'right':0,
                'front':0,
                'back':0,

        }
        limit =45
        if roll < 0:
                movements['left'] = min(1, abs(roll)/limit)    #x+
        else:
                movements['right'] = min(1, abs(roll)/limit)   #x-

        if pitch < 0:
                movements['front'] = min(1, abs(pitch)/limit)  #y+
        else:
                movements['back'] = min(1, abs(pitch)/limit)   #y-
        return movements


def simulate_key(key,on_time):
        kb_controller = Controller()
        if on_time >0:
                kb_controller.press(key)
                time.sleep(on_time)
                kb_controller.release(key)


def start_iio(device: iio.Device):
        start = time.time()
        data = get_data(device)        # partea de iio, salveaza toate valorile din canale
        roll,pitch = get_roll_pitch(data)    #   formula matematica de convertire a valorilor din canale
        movement = get_movement(roll,pitch)
        timer=time.time()- start

        for direction,key in [('front', 'w'), ('back', 's'), ('left', 'a'), ('right','d')]:
                value = movement[direction]
                if value > 0.2:
                        on_time = TIMEOUT* value
                        key_thread = Thread(target=simulate_key, args=(key,on_time))
                        key_thread.daemon = True
                        key_thread.start()
        #print(roll,pitch)
       #init_movements ={'left':0,'right': 0, 'front': 0, 'back':0}
        time.sleep(TIMEOUT- timer)
        #print(movement)
       
        return movement

def simulate_movement(queue: Queue):
        device = init_device()
        while not exit_event.is_set():
                if start_event.is_set():
                        queue.put(start_iio(device))
                       # print("running") 
                        #  mvm = queue.get()
                        #  mvm['left'] +=1   
                        #  queue.put(mvm)   
                        #queue.get()
                        #increment movement left value
                        #time.sleep(1)
                start_event.wait()

       

def on_keypress(key):

#dam start la start event si exit event
     #daca apasam ctrl intra intr-un if care seteaza start eventul
        #keyboard.key.ctrl_l
        #start_event.is_set() return bool
        #start_event.clear()    unsets event
        #start_event.set()      sets event

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
        

        return True


if __name__ =='__main__':
        time.sleep(1)
        start_event = Event()   #cream eventuri globale(fiindca sunt in main)
        exit_event = Event()              #pot si setate sau nu       
                                #asteapta sa fie setat din afara si dupa trece mai departe
        

        init_movements ={'left':0,'right': 0, 'front': 0, 'back':0}
        movement_q = Queue()
        movement_q.put(init_movements)

        #incepem 2 thread-uri - g
        gui_thread = Thread(target=create_gui, args=(movement_q,))   #creeaza interfata
        gui_thread.daemon = True
        gui_thread.start()

        iio_thread = Thread(target=simulate_movement, args=(movement_q,))   #simuleaza si citeste
        iio_thread.daemon = True
        iio_thread.start()

        print("Press Left CTRL to start/stop applicatioon. Press Esc to exit")

        with keyboard.Listener(on_press=on_keypress) as listener:
                listener.join()


#WAWAwawawawawawawawawawa

