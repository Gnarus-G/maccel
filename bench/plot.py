import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

control_data = pd.read_csv("control.csv")
old_data = pd.read_csv("usbmouse.csv")
new_data = pd.read_csv("input_handler.csv")

fig, ax = plt.subplots()

ax.plot(np.arange(1000), control_data["diff"], 'b',
        label=f"Hid-generic (mean = {control_data["diff"].mean()}us)")

ax.plot(np.arange(1000), old_data["diff"], 'r',
        label=f"Old driver   (mean = {old_data["diff"].mean()}us)")

ax.plot(np.arange(1000), new_data["diff"], 'g',
        label=f"New driver  (mean = {new_data["diff"].mean()}us)")

plt.ylabel('lag (us)')
plt.xlabel('reads')

legend = ax.legend(loc='upper center', fontsize='x-large')

plt.show()

""" Some extra data points out of curiosity """

virtual_minus_source = new_data["virtual_event_time"] - new_data["event_time"]
print("average lag between source and virtual device:",
      virtual_minus_source.mean())

new_data["read_minus_virtual"] = new_data["read_time"] - \
    new_data["virtual_event_time"]
print("average lag between virtual device and read:",
      new_data["read_minus_virtual"].mean())
