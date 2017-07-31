#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy as np
import sys
import json
import os
import glob

p50 = []
p75 = []
p95 = []
p99 = []
tup = []

for json_file in glob.glob("experiment_*/*.json"):
	with open(json_file, "r") as infile:
		json_data = json.load(infile)
		#data.append( (float(json_data['results']['medianFCT'].split()[0]),  (json_data['params']['flowletGap'], json_data['params']['dctcpG']) ) )
		tup.append( (json_data['params']['flowletGap'], json_data['params']['dctcpG']) )
		p50.append(float(json_data['results']['medianFCT'].split()[0]))
		p95.append(float(json_data['results']['p95'].split()[0]))
		p99.append(float(json_data['results']['p99'].split()[0]))

N = len(p50)
ind = np.arange(N)
width = 0.35

p50_bar = plt.bar(ind, p50, width, color='r', label='p50')
p95_bar = plt.bar(ind, p95, width, color='g', label='p95', bottom=p50)
p99_bar = plt.bar(ind, p95, width, color='b', label='p99', bottom=p95)

labels = [str(x) for x in tup]
plt.xticks(ind + width/2, labels, rotation='vertical')
#plt.xticklabels(labels, rotation='vertical')
#plt.ylim((0,0.2))
plt.xlabel("(flowlet_gap (us), DCTCP shift_g) - 0 = disabled")
plt.ylabel("Average FCT (ms)")
plt.title("Flowlet gap vs DCTCP shift_g - Incast")
plt.legend()
plt.savefig("flowlet_dctcp_perc.png")
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


