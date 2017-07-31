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
		data.append( (float(json_data['results']['averageFCT'].split()[0]),  (json_data['params']['flowletGap'], json_data['params']['dctcpG']) ) )

fct = []
tup = []

for point in sorted(data):
	fct.append(point[0])	
	tup.append(point[1])

print(fct)
print(tup)


#x = range(len(flowlet_gap))
#plt.plot(fct, 'ro', color='green', label='custom')
#plt.xticks(x, flowlet_gap, rotation='vertical')
## Labels
#plt.xlabel("ExpID")
#plt.ylabel("FCT (ms)")
##plt.ylim((0,5))
#
## Axes and ticks
##plt.margins(0.2)
#plt.legend()
#plt.grid(True)
#plt.show()


