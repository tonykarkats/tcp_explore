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
		data.append( (json_data['params']['flowletGap'], float(json_data['results']['averageFCT'].split()[0])) ) 

flowlet_gap = []
fct = []

for point in sorted(data):
	flowlet_gap.append(point[0])	
	fct.append(point[1])

print(flowlet_gap)
print(fct)
print("VAR of FCT = {}".format(np.var(fct)))


x = range(len(flowlet_gap))
plt.plot( fct, 'ro', color='green', label='custom')
plt.xticks(x, flowlet_gap)
## Labels
#plt.xlabel("ExpID")
#plt.ylabel("FCT (ms)")
plt.ylim((0,0.2))
#
## Axes and ticks
#plt.margins(0.2)
#plt.legend()
plt.grid(True)
plt.show()


