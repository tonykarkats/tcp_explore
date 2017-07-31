#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy as np
import sys
import json
import os
import glob

data = []

for json_file in glob.glob("experiment_*/*.json"):
	with open(json_file, "r") as infile:
		json_data = json.load(infile)
		data.append( (json_data['params']['tcpTimestamp'], float(json_data['results']['averageFCT'].split()[0])*1000 ) )

fct = []
tup = []

for point in sorted(data):
	fct.append(point[1])	
	tup.append(point[0])

N = len(fct)
ind = np.arange(N)
width = 0.15

fig, ax = plt.subplots()
ax.bar(ind, fct, width, color="red")

labels = [str(x) for x in tup]
ax.set_xticks(ind + width/2)
ax.set_xticklabels(labels)
plt.margins(0.3, 0)
plt.ylim((0, 1400))
plt.xlabel("tcp_timestamps")
plt.ylabel("Average FCT (ms)")
plt.grid()
plt.savefig("sens_timestamps.pdf", format='pdf', dpi=1200)
plt.show()
