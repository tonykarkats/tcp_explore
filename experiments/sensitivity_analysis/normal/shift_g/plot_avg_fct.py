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
		data.append( (json_data['params']['dctcpG'], float(json_data['results']['averageFCT'].split()[0])*1000 ) )

fct = []
tup = []

for point in sorted(data):
	fct.append(point[1])	
	tup.append(point[0])

N = len(fct)
ind = np.arange(N)
width = 0.35

fig, ax = plt.subplots()
ax.bar(ind, fct, width)

labels = [str(x) for x in tup]
ax.set_xticks(ind + width/2)
ax.set_xticklabels(labels)
plt.xlabel("dctcp_shift_g")
plt.ylabel("Average FCT (ms)")
plt.grid()
plt.savefig("sens_shift_g_normal.pdf", format='pdf', dpi=1200)
plt.show()
