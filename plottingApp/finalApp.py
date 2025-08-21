from queue import Queue
from threading import Thread, Event
from pynput import keyboard 
import time
import math
import iio
from pynput.keyboard import Controller
import tkinter as tk

URI="ip:10.76.84.231"
DEVICE="IIO_ADC_AD5592R_S"
TIMEOUT=0.3

def on_keypress(key):
        if (key == keyboard.Key.ctrl_l) and (not start_event.is_set()):
                start_event.set()
                print("Am setat")
                return True
        if (key == keyboard.Key.ctrl_l) and (start_event.is_set()):
                start_event.clear()
                print("Am sters")
                return True
        if key==keyboard.Key.esc:
                exit_event.set()
                print("exit")
                return False

def create_gui(queue):
        def update_sq_color(canvas, sq, value):
                        value = max(0, min(1, value))
    

                        red_intensity = int(255 * value)
    

                        green_intensity = int(255 * (1 - value))
                        blue_intensity = int(255 * (1 - value))
    

                        color = f"#{red_intensity:02x}{green_intensity:02x}{blue_intensity:02x}"

                        canvas.itemconfig(sq, fill=color)

        def update_gui():
                movement=queue.get()

                update_sq_color(canvas,sqares['left'],movement['left'])
                update_sq_color(canvas,sqares['right'],movement['right'])
                update_sq_color(canvas,sqares['front'],movement['front'])
                update_sq_color(canvas,sqares['back'],movement['back'])
               
                root.after(int(TIMEOUT*1000),update_gui)

        root=tk.Tk()
        root.title("key simulator")

        canvas=tk.Canvas()
        canvas=tk.Canvas(root,width=400,height=400)
        canvas.pack()

        

        square_size=100

        sqares={
                'left':canvas.create_rectangle(50,150,50+square_size,150+square_size,fill='white'),
                'right':canvas.create_rectangle(250,150,250+square_size,150+square_size,fill='white'),
                'front':canvas.create_rectangle(150,50,150+square_size,50+square_size,fill='white'),
                'back':canvas.create_rectangle(150,250,150+square_size,250+square_size,fill='white'),
        }

        update_gui()

        root.mainloop()

def init_device():
        ctx=iio.Context(URI)
        device=ctx.find_device(DEVICE)

        if device is None:
                raise ValueError("No device")
        return device

def get_data(device: iio.Device):
        channels_name=[f'voltage{i}' for i in range(6)]
        axis=['x','y','z']
        axis_data={
                'x':{"-":int,'+':int},
                'y':{"-":int,'+':int},
                'z':{"-":int,'+':int},
        }

        for i, channels_name in enumerate(channels_name):
                channel = device.find_channel(channels_name)
                if channel is None:
                        raise ValueError("Channel not found")
                attr=channel.attrs['raw'].value
                if i%2 == 0:
                        axis_data[axis[i//2]]['+']=int(attr)
                else:
                        axis_data[axis[i//2]]['-']=int(attr)
        #time.sleep(1)
        #print(axis_data)ssssssssssssssssssssssssssssssssssassssssawawasassssssssdssawwwwwwwwwwwwwawassssssss
        return axis_data
        

def get_roll_pitch(data):
        x_accel=data['x']['+']-data['x']['-']
        y_accel=data['y']['+']-data['y']['-']
        z_accel=data['z']['+']-data['z']['-']

        pitch=math.atan2(y_accel,z_accel)*180/math.pi
        roll=math.atan2(-x_accel,math.sqrt(y_accel**2 + z_accel**2))*180 / math.pi

        return roll,pitch

def get_movement(roll,pitch):
        movements={
                'left':0,
                'right':0,
                'front':0,
                'back':0
        }
        
        if pitch<0:
                movements['front']=min(1,abs(pitch/45))
        else :
                movements['back']=min(1,abs(pitch/45)-0.3)

        if roll<0:
                movements['left']=min(1,abs(roll)/45)
        else:
                movements['right']=min(1,abs(roll)/45)

        return movements
def simulate_key(key,on_time):
        kb_controller=Controller()

        if on_time>0:
                kb_controller.press(key)
                time.sleep(on_time)
                kb_controller.release(key)

def start_iio(device: iio.Device):
        start=time.time()

        data =get_data(device)
        roll,pitch=get_roll_pitch(data)
        movement = get_movement(roll,pitch)
        timer =time.time()-start

        for direction,key in[('front','w'),('back','s'),('left','a'),('right','d')]:
                value=movement[direction]
                if value>0.2:
                        on_time=TIMEOUT*value
                        key_thread=Thread(target=simulate_key,args=(key,on_time))
                        key_thread.daemon=True
                        key_thread.start()


        time.sleep(TIMEOUT-timer)
        return movement



def simulate_movement(queue: Queue):
        device=init_device()

        while not exit_event.is_set():
                if start_event.is_set():
                        queue.put(start_iio(device))
                        # print("Running")
                        # mvm=queue.get()
                        # mvm['left']+=1
                        # queue.put(mvm)
                        # time.sleep(1)
                start_event.wait()





if __name__ == '__main__':
        start_event=Event()
        exit_event=Event()
        
        init_movments={'left':0 , 'right':0, 'front':0, 'back':0}
        
        movment_q=Queue()
        movment_q.put(init_movments)


        gui_thread = Thread(target=create_gui, args=(movment_q,))
        gui_thread.daemon = True
        gui_thread.start()

        iio_thread = Thread(target=simulate_movement,args=(movment_q,))
        iio_thread.daemon = True
        iio_thread.start()


        print("Press left Ctrl to start/stop application . Press Esc to exit")

        with keyboard.Listener(on_press=on_keypress) as listener:
                listener.join()



#ssssasdsssdsdss