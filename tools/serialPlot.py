# Plotting serial data collected from ESP32

import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from threading import Thread

class serialPlot:
    
    def __init__(self, sPort, sBaudRate, sTimeout, n):
        self.port = sPort
        self.baudRate = sBaudRate
        self.timeout = sTimeout
        self.data = []
        self.x_vals = []
        self.y_vals = []
        self.n = n # Number of data entries

        print(f"\nStarting serial plot. Connecting to {sPort} with baud rate configured at {sBaudRate} bps..")
        try:
            self.serialConn = serial.Serial(port=sPort, baudrate=sBaudRate, timeout=sTimeout)
            print("\nConnection successful!")
        except:
            print(f"\nFailed to connect with {sPort}.")
            exit()

        self.thread = Thread(target=self.readSerial, daemon=True)
        self.thread.start()

    def readSerial(self):
        while True:
            line = self.serialConn.readline().decode("utf-8").strip()
            if line:
                self.data = line.split()

    def animate(self, frame):
        if len(self.data) >= self.n: # Ensure full UART message has been read
            try:
                self.x_vals.append(float(self.data[0]))
                self.y_vals.append(float(self.data[1]))
                plt.cla()  # Clear the previous frame
                plt.plot(self.x_vals, self.y_vals)
                print(self.x_vals)
            except ValueError:
                pass  # Ignore lines that can't be converted

speedPlot = serialPlot('COM3', 115200, 1, 4)
ani = FuncAnimation(plt.gcf(), speedPlot.animate, interval=1000, cache_frame_data=False)
plt.show()
