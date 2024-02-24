# CMPT 433 - Light Dip Detector Testing GUI
# Created by Brian Fraser
# Version 1.0 - Created Feb 6, 2021
#
# See comments below for sources.
#
# SETUP (Linux):
#   sudo apt-get install python3 python3-tk
#   sudo apt-get install python3-matplotlib
# RUN
#  python3 as2UdpGui.py

import socket
import _thread
from tkinter import *
from tkinter import ttk
from matplotlib.figure import Figure 
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

SEND_BUTTON_TEXTS = ["help", "?", "count", "length", "dips", "history", "", "stop"]

# ****************************************
#  UDP SOCKET TX/RX
# ****************************************
sock = 0    # UDP socket (global)

# Written with lots of help from ChatGPT!
def openPort(): 
    global sock
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def sendUDPMessage(msg):
    UDP_IP = "192.168.7.2"
    UDP_PORT = 12345
    global sock, rxText
    print("TX message:\n", msg)
    rxText.set("")
    sock.sendto(msg.encode(), (UDP_IP, UDP_PORT))

    # Update the graph (on main thread)
    root.after(100, refresh_ui)

def makeSendFunc(msg):
    return lambda : sendUDPMessage(msg + "\n")    

def is_float(string):
    try:
        float(string)
        return True
    except ValueError:
        return False

def rxTextToData(rxText):
    rxText = rxText.replace(" ", ",")
    rxText = rxText.replace("\n", ",")
    rxText = rxText.replace("\r", ",")
    rxText = rxText.strip(", \n")
    asList = rxText.split(",")
    data = []
    for str in asList:
        if is_float(str):
            data.append(float(str))
    return data

# Listen for incoming messages on UDP port
# SOURCE: https://stackoverflow.com/a/66411955
def listening_thread():
    global data     # data needs to be defined as global inside the thread
    global rxText
    while True:
        data_raw, addr = sock.recvfrom(4096)
        data = data_raw.decode()    # My test message is encoded
        print (data)
        display = (rxText.get() + "\n" + data).strip()
        rxText.set(display)
        data = rxTextToData(rxText.get())
        print("DATA: \n", data)

def refresh_ui():        
    global data     # data needs to be defined as global inside the thread
    if len(data) > 0:
        plot_data(data)
    data = []


# ****************************************
#  Graph using Matplotlib
# ****************************************

# SOURCE: https://www.geeksforgeeks.org/how-to-embed-matplotlib-charts-in-tkinter-gui/
def plot_data(data):
    # the figure that will contain the plot 
    fig = Figure(figsize = (5, 5), 
                 dpi = 100) 
    
    # Set title and axis text
    fig.suptitle('History Voltage Samples')
    fig.text(0.01, 0.5, 'Voltage', va='center', rotation='vertical')
    fig.text(0.5, 0.04, 'Samples', ha='center')
  
    # adding the subplot 
    plot = fig.add_subplot(111) 

    # Set vertical axis range
    plot.set_ylim(0, 1.9)

    # plotting the graph 
    plot.plot(data) 
  
    # creating the Tkinter canvas containing the Matplotlib figure 
    plotFrame = ttk.Frame(mainframe)
    plotFrame.grid(column=5, row=1, sticky=(N, W, E))
    canvas = FigureCanvasTkAgg(fig, 
                               master = plotFrame)
    canvas.draw() 
  
    # placing the canvas on the Tkinter window 
    canvas.get_tk_widget().pack() 

# ****************************************
# Initialize and Create GUI
# ****************************************    
openPort()

try:
   _thread.start_new_thread(listening_thread, ())
except:
    print ("Error: unable to start thread")
    quit()

# Setup the window
root = Tk()
root.title("As2 UDP GUI")

mainframe = ttk.Frame(root, padding="3 3 12 12")
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=1)

# Text area to show received messages
ttk.Label(mainframe, text="Rx Text").grid(column=1, row=0, sticky=(W, E))
rxText = StringVar()
rxText.set("... <no text yet> ...")

rxTextFrame = ttk.Frame(mainframe, 
        borderwidth=5,
        padding="3 3 12 12",
        width=700,
        height=1000,
        relief="ridge"
)
rxTextFrame.grid_propagate(False)
rxTextFrame.grid(
        column=1, row=1, 
        rowspan=len(SEND_BUTTON_TEXTS), sticky=(W, E))
rxTextLabel = ttk.Label(
        rxTextFrame, 
        textvariable=rxText,
        font="TkFixedFont",
)
rxTextLabel.grid(
        column=0, 
        row=0, 
        sticky=(W, E)
)

# Buttons to send commands
ttk.Label(mainframe, text="Tx Text").grid(column=0, row=0, sticky=(W, E))
btnFrame = ttk.Frame(mainframe, padding="3 3 12 12")
btnFrame.grid(column=0, row=1, sticky=(N, W, E))
for i, msg in enumerate(SEND_BUTTON_TEXTS):
    cmd = makeSendFunc(msg)
    ttk.Button(btnFrame, text=msg, command=cmd).grid(column=0, row=i, sticky=W)
root.bind('<Return>', lambda event=None: sendUDPMessage("history\n"))
  
# Show the plot initially.
plot_data([0])

# Space out all widgets
for child in mainframe.winfo_children(): 
    child.grid_configure(padx=5, pady=5)

# Run the window loop
root.mainloop()
