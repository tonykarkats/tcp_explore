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
		data.append( (float(json_data['results']['medianFCT'].split()[0]),  (json_data['params']['flowletGap'], json_data['params']['dctcpG']) ) )

fct = []
tup = []

for point in sorted(data):
	fct.append(point[0])	
	tup.append(point[1])

print(fct)
print(tup)

N = len(fct)
ind = np.arange(N)
width = 0.35

fig, ax = plt.subplots()
ax.bar(ind[:50], fct[:50], width)
#plt.margins(0.2)

labels = [str(x) for x in tup]
ax.set_xticks(ind[:50] + width/2)
ax.set_xticklabels(labels[:50], rotation='vertical')
#plt.ylim((0,0.2))
plt.xlabel("(flowlet_gap (us), DCTCP shift_g) - 0 = disabled")
plt.ylabel("Average FCT (ms)")
plt.title("Flowlet gap vs DCTCP shift_g - Incast")
plt.savefig("flowlet_dctcp_incast.png")
plt.show()




#x = range(len(flowlet_gap))
#plt.plot(fct, 'ro', color='green', label='custom')
#plt.xticks(x, flowlet_gap, rotation='vertical')
## Labels
#plt.xlabel("ExpID")
#plt.ylabel("FCT (ms)")
#
## Axes and ticks
##plt.margins(0.2)
#plt.legend()
#plt.grid(True)
#plt.show()


