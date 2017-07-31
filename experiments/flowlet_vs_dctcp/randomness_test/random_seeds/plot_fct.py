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
		data.append( (json_data['setup']['expId'], float(json_data['results']['averageFCT'].split()[0])) ) 

exp_id = []
fct = []

for point in sorted(data):
	exp_id.append(point[0])	
	fct.append(point[1])

print(exp_id)
print(fct)
print("VAR = {}".format(np.var(fct)))


x = range(len(exp_id))
plt.plot( fct, 'ro', color='green', label='custom')
plt.xticks(x, exp_id, rotation='vertical')
## Labels
plt.xlabel("Experiment ID")
plt.ylabel("Average FCT (ms)")
plt.ylim((0,0.2))
#
## Axes and ticks
#plt.margins(0.2)
#plt.legend()
plt.grid(True)
plt.show()


