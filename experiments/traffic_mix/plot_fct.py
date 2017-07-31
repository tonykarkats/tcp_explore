#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy as np
import sys
import json
import os

experiments_folder = sys.argv[1]

custom_data = []
conga_data = []

for json_file in os.listdir(experiments_folder +  "/custom"):
	with open(experiments_folder + "/custom/" + json_file, "r") as infile:
		json_data = json.load(infile)
		custom_data.append(((float(json_data['results']['averageFCT'].split()[0])),
					 json_data['setup']['expId']))

for json_file in os.listdir(experiments_folder +  "/conga"):
	with open(experiments_folder + "/conga/" + json_file, "r") as infile:
		json_data = json.load(infile)
		conga_data.append(((float(json_data['results']['averageFCT'].split()[0])),
					 json_data['setup']['expId']))

conga_fct = []
custom_fct = []
exp_id = []

for point in sorted(conga_data):
	conga_fct.append(point[0]*1000)
	exp_id.append(point[1])	
	for data in custom_data:
		if (data[1] == point[1]):
			custom_fct.append(data[0]*1000)

print(custom_fct)
print(conga_fct)
print(exp_id)


x = range(len(exp_id))
plt.plot(conga_fct, 'ro', color='red', label='conga')
plt.plot(custom_fct, 'ro', color='green', label='custom')
plt.xticks(x, exp_id, rotation='vertical')
# Labels
plt.xlabel("ExpID")
plt.ylabel("FCT (ms)")
plt.ylim((0,5))

# Axes and ticks
#plt.margins(0.2)
plt.legend()
plt.grid(True)
plt.savefig("conga.png", dpi=200)
plt.show()


