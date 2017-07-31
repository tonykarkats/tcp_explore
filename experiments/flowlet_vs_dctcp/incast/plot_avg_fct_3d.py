#!/usr/bin/python

from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
from matplotlib import cm
import numpy as np
import sys
import json
import os
import glob
from matplotlib.ticker import LinearLocator, FormatStrFormatter

fct = []
flowlet_gap = []
shift_g = []

for json_file in glob.glob("experiment_*/*.json"):
	with open(json_file, "r") as infile:
		json_data = json.load(infile)
		i = json_data['params']['flowletGap']
		if int(i) > 10:
			continue
		flowlet_gap.append(json_data['params']['flowletGap'])
		fct.append(float(json_data['results']['averageFCT'].split()[0])*1000.0)
		shift_g.append(json_data['params']['dctcpG'])

print(fct)
print(flowlet_gap)
print(shift_g)

#fct = fct[:20]
#flowlet_gap = flowlet_gap[:20]
#shift_g = shift_g[:20]

fig, ax = plt.subplots()
scat = ax.scatter(shift_g, flowlet_gap, c=fct, s=150, marker='o', cmap=cm.coolwarm)
#ax = fig.gca(projection='3d')

#flowlet_gap, shift_g = np.meshgrid(flowlet_gap, shift_g)

#surf = ax.plot_surface(flowlet_gap, shift_g, fct, cmap=cm.coolwarm,
#					   linewidth=0, antialiased=False)

# Customize the z axis.
#ax.set_zlim(-1.01, 1.01)
#ax.zaxis.set_major_locator(LinearLocator(10))
#ax.zaxis.set_major_formatter(FormatStrFormatter('%.02f'))

# Add a color bar which maps values to colors.
fig.colorbar(scat, shrink=0.5, aspect=5, label='Average FCT (ms)')


#plt.ylim((0,0.2))
plt.ylabel("Flowlet Gap (us)")
plt.xlabel("DCTCP shift_g")
#plt.zlabel("Average FCT")
#plt.title("Flowlet gap vs DCTCP shift_g - Incast (Low Load)")
plt.savefig("flowlet_dctcp_incast_normal_load_scatter.pdf", format='pdf', dpi='1200')
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


