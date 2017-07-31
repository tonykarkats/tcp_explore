#!/usr/bin/python

import subprocess
import glob
import numpy as np
import os
import pickle
import json
import time

# This main function of this file (run_experiment) runs one experiment with the given setup 
# and params.

# Parses a file as produced by DCE and gets the FCT and TPS of the iperf flow
# WARNING: For the function to function properly only iperf should appear on stdout

def parse_iperf_client_file(filename):
	with open(filename, "r") as infile:
		lines = infile.readlines()
		if (len(lines) == 0):
			return -1.0, -1.0
		if (lines[1].startswith("Client")):
			fct = float(lines[-1].split()[2].split("-")[1])
			tps = float(lines[-1].split()[6])
			if lines[-1].split()[7] == 'Kbits/sec':
				tps /= 1000.0
			elif lines[-1].split()[7] == 'Gbits/sec':
				tps *= 1000.0
			return fct, tps
		else:
			return -1.0, -1.0

# Runs an experiment with the given setup and params list and outputs
# a JSON report describing it.

def run_experiment(pickle_file, output_folder):

	unpickled = pickle.load(open(pickle_file, "rb"))
	setup = unpickled[0]
	params = unpickled[1]
	
	errors = {}
	subprocess.call("rm -rf files-* elf-cache *.pcap", shell=True);
	command = "./waf --run \"dce-fat-tree "
	for key, val in setup.iteritems():
		command += "--{}={} ".format(key, val)

	for key, val in params.iteritems():
		command += "--{}={} ".format(key, val)

	command += "\""

	start = time.time()
	ret = subprocess.call(command, shell=True)
	if ret != 0:
		print("Experiment {} finished abnormally!".format(pickle_file))
		exit(-1)
	elapsed = (time.time() - start)

	# Parse the results
	fct_list = []
	tps_list = []
	iperfs_not_ended = 0
	for file_folder in glob.glob('files-*'):
		rootdir = file_folder + "/var/log"
		for subdir, dirs, files in os.walk(rootdir):
			for file in files:
				if (file == "stdout"):
					fct ,tps = parse_iperf_client_file(os.path.join(subdir, file))
					if (fct != -1.0):
						fct_list.append(fct)
						tps_list.append(tps)
				elif (file == "stderr"):
					# Error file should be empty
					if (len(open(os.path.join(subdir, file)).readlines()) != 0):
						print("{} has reported an iperf error".format(os.path.join(subdir,file)))
						errors[os.path.join(subdir,file)] = "iperf error"
				elif (file == "status"):
					for line in open(os.path.join(subdir, file)).readlines():
						if ("Never ended." in line):
							iperfs_not_ended += 1
				
	# Reporting

	errors['iperfs not ended'] = iperfs_not_ended - (setup['k']**3) / 4
	
	results = {}
	results['simTime'] = elapsed
	results['numFlows'] = len(fct_list)
	results['averageFCT'] = str(np.average(fct_list)) + ' sec'
	results['medianFCT'] = str(np.median(fct_list)) + ' sec'
	results['p75'] = str(np.percentile(fct_list, 75)) + ' sec'
	results['p95'] = str(np.percentile(fct_list, 95)) + ' sec'
	results['p99'] = str(np.percentile(fct_list, 99)) + ' sec'
	results['averageTPS'] = str(np.average(tps_list)) + ' Mbits/sec'

	# Also output the flow statistics to a file
	with open("flow_stats.out", "w") as flow_stats_file:
		flow_stats_file.write("FCT\t\tTPS\n------\n");
		for fct, tps in zip(fct_list, tps_list):
			flow_stats_file.write(str(fct) + "\t" + str(tps) + "\n")

	# Queue statistics
	with open("queue_stats.out", "r") as infile:
		lines = infile.readlines()
		results['packetsReceived'] = int(lines[-2].split()[4])
		results['packetsDropped'] = int(lines[-1].split()[4])
  
	report = {"setup" : setup, "params" : params, "results" : results, "errors" : errors}
	with open("experiment_{}.json".format(setup['expId']), "w") as outfile:
		outfile.write(json.dumps(report, indent=4, sort_keys=True))

	# Create the folder and move in all the relevant files
	subprocess.call("mkdir experiment_{}".format(setup['expId']), shell=True)
	subprocess.call("mv experiment_{}.json queue_stats.out flow_stats.out experiment_{}".format(setup['expId'], setup['expId']), shell=True)
	subprocess.call("mv experiment_{} {}".format(setup['expId'], output_folder), shell=True)

	# Return the results
	return results

##############################################################################
