#!/usr/bin/python

# This script implements the exploration of the parameter space, using the Simulated Annealing
# algorithm.

from random import random
from experiment import *
from math import exp, log, ceil

import subprocess
import glob
import numpy as np
import os
import pickle
import json
import time

# The setup of the experiment, i.e. parts that remain mostly static.

setup = { "k" 			 : 8,
		  "simTime" 	 : 15,
          "expType"      : "cdf",
          "dataRate"     : "1000Mbps",
          "queueSize"    : 200,
          "probSamePod"  : 0.3,
          "probSameEdge" : 0.2,
          "flowsPerSec"  : 50,
          "flowSize"     : 128000,
          "delay"        : 50,
		  "distrFile"	 : "distributions/conga.txt"
}

# This dictionary specifies the parameter properties for the random exploration.
# Each key(parameter) maps to a list of the form [<start_value>, <low_value>, <high_value>, <step>]

param_properties = {
		"tcpSndBufSize"				   : [256000, 128000, 1024000, 128000],
		"tcpRcvBufSize" 			   : [256000, 128000, 1024000, 128000],
		"tcpTcpNoDelay"				   : [1, 0, 1, 1],
		"tcpWindowScaling"			   : [1, 0, 1, 1],
		"tcpSack"					   : [1, 0, 1, 1],
		"tcpFack"					   : [1, 0, 1, 1],
		"tcpDsack"					   : [1, 0, 1, 1],
		"tcpTimestamp"				   : [1, 0, 1, 1],
		"tcpLowLatency"				   : [0, 0, 1, 1],
		"tcpAbortOnOverflow"		   : [1, 0, 1, 1],
		"tcpNoMetricsSave"			   : [1, 0, 1, 1],
		"tcpSlowStartAfterIdle"		   : [0, 0, 1, 1],
		"tcpReordering"				   : [3, 1, 10, 1],
		"tcpRetries1"				   : [3, 1, 10, 1],
		"tcpEarlyRetrans"			   : [2, 0, 4, 1],
		"tcpMinRto"					   : [1000, 1000, 1000000, 1000],
		"tcpReTxThreshold"			   : [3, 2, 5, 1],
		"tcpFinTimeout"				   : [30, 10, 100, 5],
		"tcpFrto"					   : [1, 0, 1, 1],
		"tcpTwReuse"				   : [1, 0, 1, 1],
		"ecnThresh"					   : [10, 5, 40, 5],
		"dctcpEnable"				   : [1, 0, 1, 1],
		"dctcpG"					   : [4, 0, 6, 1],
        "flowletGap"				   : [10, 0, 40, 1],
		"tcpInitCwnd"				   : [2, 1, 10, 1]
}

# Seeds for the random experiments.
# All the seeds have to be the same across experiments so that the comparison is fair.
seeds = []
for i in range(0, 5):
	seeds.append(int(4200*random()))

# Parses the stdout of iperf and returns the throughput and flow completion time.

def parse_iperf_client_file(filename):
	with open(filename, "r") as infile:
		lines = infile.readlines()
		if (len(lines) == 0):
			return -1.0, -1.0
		if (lines[1].startswith("Client")):
			try:
				fct = float(lines[-1].split()[2].split("-")[1])
			except:
				return -1.0, -1.0
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

def run_experiment(setup, params):
	
	errors = {}
	subprocess.call("rm -rf files-* elf-cache *.pcap", shell=True);
	command = "./waf --run \"dce-fat-tree "
	for key, val in setup.iteritems():
		command += "--{}={} ".format(key, val)

	for key, val in params.iteritems():
		command += "--{}={} ".format(key, val)

	command += "\""

	print(command)
	start = time.time()
	ret = subprocess.call(command, shell=True)
	if ret != 0:
		print("Experiment finished abnormally! Running next!")
		return None
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
					#print(os.path.join(subdir, file))
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
				
					

	##### Reporting

	errors['iperfs not ended'] = iperfs_not_ended - (setup['k']**3) / 4
	
	results = {}
	results['simTime'] = elapsed
	results['numFlows'] = len(fct_list)
	results['averageFCT'] = np.average(fct_list)
	results['medianFCT'] = np.median(fct_list)
	results['p75'] = np.percentile(fct_list, 75)
	results['p95'] = np.percentile(fct_list, 95)
	results['p99'] = np.percentile(fct_list, 99)
	results['averageTPS'] = np.average(tps_list)
  
	# Return the results
	return results

# Runs an experiment and calculates the objective function for a given
# set of parameters.
def cost(params):
	# Run experiment with 5 different seeds 
	valid = 0
	sum_fct = 0
	for i in range(0, 3):
		setup['seed'] = seeds[i]
		results = run_experiment(setup, params)
		print("INT RESULT = {}".format(results['averageFCT']))
		if results != None:
			sum_fct += results['averageFCT']
			valid += 1
	if (valid == 0):
		return -1
	else:
		return sum_fct/valid

def acceptance_probability(old_cost, new_cost, T):
	return exp((old_cost - new_cost)/T)

# The random neighbor of a specific state is calculated as below:
# For every parameter and with probability 33.33% we choose one of two directions or stay at the same value.

def neighbor(params):
	new_params = {}
	for param, value in params.iteritems():
		param_props = param_properties[param]
		low_limit = param_props[1]
		high_limit = param_props[2]
		step = param_props[3]
		rand = random()
		if rand < 1.0/3.0:
			# Move up
			new_params[param] = value + step if value + step <= high_limit else value 
		elif rand < 2.0/3.0:
			# Move down
			new_params[param] = value - step if value - step >= low_limit else value 
		else:
			# Don't move
			new_params[param] = value
	return new_params

def anneal(params):
	outfile = open("anneal.out", "wb")
	old_cost = cost(params)
	T = 1.0
	T_min = 0.0001
	alpha = 0.75
	step = 1
	total_steps = int(ceil(log(T_min, alpha))*5)
	while T > T_min:
		i = 1
		while i <= 5:
			outfile.write("[{}/{}] -> T = {}, i = {}, FCT = {}\n".format(step, total_steps, T, i, old_cost))
			print("[{}/{}] -> T = {}, i = {}, FCT = {}".format(step, total_steps, T, i, old_cost))
			new_params = neighbor(params)
			tmp_cost = cost(new_params)
			if (tmp_cost == -1.0):
				continue
			else:
				new_cost = tmp_cost
			ap = acceptance_probability(old_cost, new_cost, T)
			if ap > random():
				params = new_params
				old_cost = new_cost
			i += 1
			step += 1
		T = T*alpha
		to_pickle = [step, T, params, old_cost]
		pickle.dump(to_pickle, open("anneal.chk", "wb"))

	outfile.write("FINAL RESULTS\n-------------------\n")
	outfile.write("[{}/{}] -> T = {}, i = {}, FCT = {}\n".format(step, total_steps, T, i, old_cost))
	outfile.write("OPTIMAL PARAMETERS FOUND\n------------------------\n")
	json.dump(params, outfile)
	outfile.close()
	return params, old_cost

# Use this function in case the program crashes.
# This function restores the exploration process from the last checkpoint.

def anneal_continue():
	outfile = open("anneal.out", "a")
	unpickled = pickle.load(open("anneal.chk", "r"))
	step = unpickled[0]
	T = unpickled[1]
	params = unpickled[2]
	old_cost = unpickled[3]
	T_min = 0.0001
	alpha = 0.75
	total_steps = int(ceil(log(T_min, alpha))*5)
	while T > T_min:
		i = 1
		while i <= 5:
			outfile.write("[{}/{}] -> T = {}, i = {}, FCT = {}\n".format(step, total_steps, T, i, old_cost))
			print("[{}/{}] -> T = {}, i = {}, FCT = {}".format(step, total_steps, T, i, old_cost))
			new_params = neighbor(params)
			tmp_cost = cost(new_params)
			if (tmp_cost == -1.0):
				continue
			else:
				new_cost = tmp_cost
			ap = acceptance_probability(old_cost, new_cost, T)
			if ap > random():
				params = new_params
				old_cost = new_cost
			i += 1
			step += 1
			to_pickle = [step, T, i, params, old_cost]
			pickle.dump(to_pickle, open("anneal.chk", "wb"))
		T = T*alpha

	outfile.write("FINAL RESULTS\n-------------------\n")
	outfile.write("[{}/{}] -> T = {}, i = {}, FCT = {}\n".format(step, total_steps, T, i, old_cost))
	outfile.write("OPTIMAL PARAMETERS FOUND\n------------------------\n")
	json.dump(params, outfile)
	outfile.close()
	return params, old_cost
	
		
if __name__ == '__main__':
	print("Generating starting parameters according to expert values..")
	starting_params = {}
	for param, props in param_properties.iteritems():
		starting_params[param] = props[0]

	anneal(starting_params)
	#anneal_continue()
