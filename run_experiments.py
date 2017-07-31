#!/usr/bin/python

# Runs all the experiments in a folder (experiments have to be in the standard form of pickle files)

from experiment import *
import sys
import os
import glob

def run_experiments(pickle_folder):
	experiments = glob.glob(pickle_folder+"/*.p")
	exp = 1
	for pickle_file in experiments:
		print("Running {} [{}/{}]".format(pickle_file, exp, len(experiments)))
		#run_experiment(pickle_folder + "/" + pickle_file, pickle_folder)
		run_experiment(pickle_file, pickle_folder)
		exp += 1

if (__name__ == "__main__"):
	run_experiments(sys.argv[1])

