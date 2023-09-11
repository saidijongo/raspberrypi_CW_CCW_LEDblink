import serial
import threading
from tkinter import *
from tkinter import ttk
import time

def send_command(command):
    ser = serial.Serial('/dev/ttyACM0', 9600)
    time.sleep(2)
    ser.write(command.encode())
    time.sleep(5)
    ser.close()

def send(*args):
    data = rpm.get() + direc.get() + degree.get()
    pos.set(pos.get() + int(degree.get()))
    print(data)

    command = f'ST,0,MOVE,0,{degree.get()},{rpm.get()},ED\r\n'
    send_command(command)

def rotate_cw_90():
    send_command('ST,0,MOVE,0,90,80,ED\r\n')

def rotate_ccw_90():
    send_command('ST,0,MOVE,0,-90,80,ED\r\n')

def lampOnOff():
    pass

def receive_status():
    send_command('S')

def initializeMotor():
    send_command('ST,0,INIT,0,ED\r\n')

root = Tk()
root.title("Stepper Motor Control")
root.configure(bg="red")

# Define variables
rpm = StringVar()
degree = StringVar()
direc = StringVar()
pos = IntVar()
status_text = StringVar()

# Create mainframes to separate sections
input_frame = ttk.Frame(root, style="My.TFrame")
input_frame.grid(column=0, row=0, sticky=(N, W, E))
output_frame = ttk.Frame(root, style="My.TFrame")
output_frame.grid(column=0, row=1, sticky=(N, W, E))

style = ttk.Style()
style.configure("My.TFrame", background="red")

# Input section
ttk.Label(input_frame, text="Speed").grid(column=1, row=1, sticky=W)
ttk.Label(input_frame, text="rpm").grid(column=3, row=1, sticky=W)
ttk.Label(input_frame, text="Direction").grid(column=4, row=1, sticky=W)
ttk.Label(input_frame, text="Angle").grid(column=6, row=1, sticky=E)
ttk.Label(input_frame, text="degree").grid(column=8, row=1, sticky=E)

feet_entry = ttk.Entry(input_frame, width=7, textvariable=rpm)
feet_entry.grid(column=2, row=1, sticky=(W, E))
degree_entry = ttk.Entry(input_frame, width=7, textvariable=degree)
degree_entry.grid(column=7, row=1, sticky=(W, E))
box = ttk.Combobox(input_frame, textvariable=direc, state='readonly')
box['values'] = ('CW', 'CCW')
box.current(0)
box.grid(column=5, row=1, sticky=(E))
send_button = ttk.Button(input_frame, text="Send", command=send)
send_button.grid(column=8, row=2, sticky=W)

# Separator line
separator = ttk.Separator(root, orient="horizontal")
separator.grid(column=0, row=2, sticky=(W, E), pady=10)

# Output section
status_label = ttk.Label(output_frame, textvariable=status_text)
status_label.grid(column=2, row=0, columnspan=2, sticky=(W, E))

status_button = ttk.Button(output_frame, text="STATUS", command=receive_status)
status_button.grid(column=2, row=1, sticky=W)

reset_button = ttk.Button(output_frame, text="INITIALIZE", command=initializeMotor)
reset_button.grid(column=3, row=1, sticky=W)

cw_button = ttk.Button(output_frame, text="CW 90", command=rotate_cw_90)
cw_button.grid(column=2, row=2, sticky=W)

ccw_button = ttk.Button(output_frame, text="CCW 90", command=rotate_ccw_90)
ccw_button.grid(column=3, row=2, sticky=W)

lamp_button = ttk.Button(output_frame, text="LAMP", command=lampOnOff)
lamp_button.grid(column=2, row=3, sticky=W)

for child in input_frame.winfo_children():
    child.grid_configure(padx=5, pady=5)

for child in output_frame.winfo_children():
    child.grid_configure(padx=5, pady=5)

root.mainloop()
