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
		data.append( (float(json_data['results']['averageFCT'].split()[0]),  (json_data['params']['flowletGap'], json_data['params']['dctcpG'], json_data['params']['ecnThresh'], json_data['setup']['queueSize'])  ) )

fct = []
tup = []

for point in sorted(data):
	fct.append(point[0])	
	tup.append(point[1])

print("Tuples are (delta, shift_g, K, Q)")
print("BEST 20 SETUPS")
print("--------------")
print(zip(fct[:20], tup[:20]))

print("WORST 20 SETUPS")
print("---------------")
print(zip(fct[-20:], tup[-20:]))

N = len(fct)
ind = np.arange(N)
width = 0.35

fig, ax = plt.subplots()
ax.bar(ind[:50], fct[:50], width)

labels = [str(x) for x in tup[:50]]
ax.set_xticks(ind[:50] + width)
ax.set_xticklabels(labels, rotation='vertical')
plt.ylim((0,0.2))
plt.xlabel("(flowlet_gap (us), DCTCP shift_g, ECN Threshold, Queue Size) - 0 = disabled")
plt.ylabel("Average FCT (ms)")
plt.title("4_tuple experiment - Incast")
plt.show()
