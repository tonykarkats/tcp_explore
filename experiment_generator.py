#!/usr/bin/python

# This script generates simple experiments in the form of a pickle file
# containing the setup and params dictionaries.
# These pickle files can then be distributed in batches to separate machines in
# order to paralellize the experiments.

# First specify the setup of the experiment (This should remain unchanged when
# exploring a specific set of parameters.

import optparse
import numpy as np
import pickle
import os

setup = { "k" 			 : 8,
		  "simTime" 	 : 20,
          "expType"      : "cdf",
          "dataRate"     : "1000Mbps",
          "queueSize"    : 200,
          "probSamePod"  : 0.3,
          "probSameEdge" : 0.2,
          "flowsPerSec"  : 100,
          "flowSize"     : 128000,
          "delay"        : 50,
		  "distrFile"	 : "distributions/custom.txt"
}

params_single = {
		"tcpSndBufSize"				   : 256000,
		"tcpRcvBufSize" 			   : 256000,
		"tcpTcpNoDelay"				   : 1,
		"tcpWindowScaling"			   : 1,
		"tcpSack"					   : 1,
		"tcpFack"					   : 1,
		"tcpDsack"					   : 1,
		"tcpTimestamp"				   : 1,
		"tcpLowLatency"				   : 0,
		"tcpAbortOnOverflow"		   : 1,
		"tcpNoMetricsSave"			   : 1,
		"tcpSlowStartAfterIdle"		   : 0,
		"tcpReordering"				   : 3,
		"tcpRetries1"				   : 3,
		"tcpEarlyRetrans"			   : 2,
		"tcpMinRto"					   : 1000000,
		"tcpReTxThreshold"			   : 3,
		"tcpFinTimeout"				   : 30,
		"tcpFrto"					   : 1,
		"tcpTwReuse"				   : 1,
		"ecnThresh"					   : 10,
		"dctcpEnable"				   : 1,
		"dctcpG"					   : 5,
        "flowletGap"				   : 50,
        "tcpInitCwnd"				   : 2
}

params_optimal = {
		"tcpSndBufSize"				   : 256000,
		"tcpRcvBufSize" 			   : 256000,
		"tcpTcpNoDelay"				   : 0,
		"tcpWindowScaling"			   : 1,
		"tcpSack"					   : 1,
		"tcpFack"					   : 0,
		"tcpDsack"					   : 0,
		"tcpTimestamp"				   : 1,
		"tcpLowLatency"				   : 1,
		"tcpAbortOnOverflow"		   : 1,
		"tcpNoMetricsSave"			   : 0,
		"tcpSlowStartAfterIdle"		   : 0,
		"tcpReordering"				   : 1,
		"tcpRetries1"				   : 1,
		"tcpEarlyRetrans"			   : 0,
		"tcpMinRto"					   : 12000,
		"tcpReTxThreshold"			   : 3,
		"tcpFinTimeout"				   : 65,
		"tcpFrto"					   : 0,
		"tcpTwReuse"				   : 1,
		"ecnThresh"					   : 10,
		"dctcpEnable"				   : 1,
		"dctcpG"					   : 2,
        "flowletGap"				   : 9,
        "tcpInitCwnd"				   : 6
}

# Then specify the ranges of the parameters to be explored
param_ranges = {
		"tcpSndBufSize"				   : [128000, 1024000],
		"tcpRcvBufSize" 			   : [128000, 1024000],
		"tcpTcpNoDelay"				   : [0, 1],
		"tcpWindowScaling"			   : [0, 1],
		"tcpSack"					   : [0, 1],
		"tcpFack"					   : [0, 1],
		"tcpDsack"					   : [0, 1],
		"tcpTimestamp"				   : [0, 1],
		"tcpLowLatency"				   : [0, 1],
		"tcpAbortOnOverflow"		   : [0, 1],
		"tcpNoMetricsSave"			   : [0, 1],
		"tcpSlowStartAfterIdle"		   : [0, 1],
		"tcpReordering"				   : [2, 10],
		"tcpRetries1"				   : [1, 10],
		"tcpEarlyRetrans"			   : [0, 4],
		"tcpMinRto"					   : [1000, 1000000],
		"tcpReTxThreshold"			   : [2, 5],
		"tcpFinTimeout"				   : [1, 10],
		"tcpFrto"					   : [0, 1],
		"tcpTwReuse"				   : [0, 1],
		"ecnThresh"					   : [5, 40],
		"dctcpEnable"				   : [0, 1],
		"dctcpG"					   : [1, 5],
        "flowletGap"				   : [20, 100],
		"tcpInitCwnd"				   : [1, 10]
}

def main():
	p = optparse.OptionParser()
	p.add_option('--gentype', '-t', default="random")
	p.add_option('--number', '-n', default=100)
	p.add_option('--output_folder', '-o', default=".")

	options, arguments = p.parse_args()
	# Get last id of the experiment
	with open("last_exp_id", "r") as infile:
		expId = int(infile.readlines()[0])
	
	if (options.gentype == "random"):
		print("Generating {} samples from uniform random distribution".format(options.number))
		for i in range(int(options.number)):
			expId += 1
			setup['expId'] = expId
			params = {}
			# For each of the parameters uniformly sample a value from the given range
			for param, prange in param_ranges.iteritems():
				low = prange[0]
				high = prange[1]
				pvalue = np.random.random_integers(low, high)
				params[param] = pvalue
			
			# XXX: Even in random sampling there is no use marking the packets with ECN
			# when DCTCP is disabled
			params['dctcpEnable'] == 1
	
			# Prepare the dictionaries for pickling
			to_pickle = [setup, params]
			outfolder = options.output_folder
			pickle.dump(to_pickle, open(outfolder + "/exp{}.p".format(expId), "wb"))			
			
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))

	elif (options.gentype == "flowlet_dctcp"):
		print("Generating flowlet gap vs DCTCP g experiments")
		params = params_single
		setup['expType'] = "incast"
		for flowlet_gap in range(0, 2000, 100):
			for dctcp_shift_g in range(0, 7):
				expId += 1
				setup['expId'] = expId
				params["flowletGap"] = flowlet_gap
				params["dctcpG"] = dctcp_shift_g
				to_pickle = [setup, params]
				outfolder = options.output_folder
				pickle.dump(to_pickle, open(outfolder + "/exp{}.p".format(expId), "wb"))
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
	elif (options.gentype == "flowlet_bufsize"):
		print("Generating Flowlet Gap - Buffer Size experiments")
		#for bufsize in range(128000, 1152000, 128000):
		#	for flowlet_gap in range(0, 11):
		for i in range(1, 5):
			expId += 1
			setup['expType'] = "incast"
			setup['expId'] = expId
			params = params_single
			params["flowletGap"] = 100
			#params["tcpSndBufSize"] = bufsize
			#params["tcpRcvBufSize"] = bufsize
			to_pickle = [setup, params]
			outfolder = options.output_folder
			pickle.dump(to_pickle, open(outfolder + "/exp{}.p".format(expId), "wb"))	
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
	elif (options.gentype == "tm"):
		print("Generating TM experiments")
		for initcwnd in range(1, 7):
			expId += 1
			setup['expId'] = expId
			params = params_single
			params["tcpInitCwnd"] = initcwnd
			to_pickle = [setup, params]
			outfolder = options.output_folder
			pickle.dump(to_pickle, open(outfolder + "/exp{}.p".format(expId), "wb"))	
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
	elif (options.gentype == "single"):
		print("Generating single experiment")
		expId += 1
		params = params_single
		setup['expType'] = "stride"
		setup['expId'] = expId
		setup['k'] = 4
		to_pickle = [setup, params]
		outfolder = options.output_folder
		pickle.dump(to_pickle, open(outfolder + "/exp_{}.p".format(setup['expId']), "wb"))			
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
	elif (options.gentype == "4_tuple"):
		print("Generating 4_tuple(Q, ECN, flowlet, g) experiments")
		for Q in [5, 10, 20, 40, 80, 160]:
			for K in [Q/16, Q/8, Q/4, Q/2, Q]:
				if (K == 0): #K = 0 makes no sense
					K = 1
				for flowlet_gap in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
					for shift_g in range(1, 6):
						expId += 1
						setup['expType'] = "incast"
						setup['expId'] = expId
						params = params_single
						params['flowletGap'] = flowlet_gap
						params['dctcpG'] = shift_g
						params['ecnThresh'] = K
						setup['queueSize'] = Q
						to_pickle = [setup, params]
						outfolder = options.output_folder
						pickle.dump(to_pickle, open(outfolder + "/exp{}.p".format(expId), "wb"))	
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
	elif (options.gentype == "sensitivity"):
		print("Generating Sensitivity Analysis experiments")
		params = params_optimal
		setup['expType'] = 'incast'
		for tcpll in range(0, 2):
			expId += 1
			setup['expId'] = expId
			params['tcpNoMetricsSave'] = tcpll
			to_pickle = [setup, params]
			outfolder = options.output_folder
			pickle.dump(to_pickle, open(outfolder + "/exp_{}.p".format(setup['expId']), "wb"))			
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
	elif (options.gentype == "flowlet_gap"):
		print("Generating Flowlet Gap experiments")
		params = params_single
		setup['expType'] = 'incast'
		rtt_min = 600
		for gap in range(rtt_min, 10*rtt_min, rtt_min/2):
			expId += 1
			setup['expId'] = expId
			params['flowletGap'] = gap
			to_pickle = [setup, params]
			outfolder = options.output_folder
			pickle.dump(to_pickle, open(outfolder + "/exp_{}.p".format(setup['expId']), "wb"))			
		with open("last_exp_id", "w") as outfile:
			outfile.write(str(expId))
		

if __name__ == '__main__':
	main()
