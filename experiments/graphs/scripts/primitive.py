import json
import enum
import sys
import re
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.patheffects as mpe
import matplotlib.ticker as mticker
from matplotlib.ticker import MaxNLocator
from dataclasses import dataclass

class action(enum.Enum):
	keep_last = 1
	keep_mean = 2
	keep_everything = 3

def data_list_from_json(input_file, chosen_action):
	data = []
	samples = {}
	contents = json.loads(open(input_file, "r").read())
	# Go through each measurement
	for m in contents["measurements"]:
		t, rw = m["type"], m["rw%"]
		rthreads, threads = m["rthreads"], m["threads"]
		rlat, wlat, bw = m["rlat"], m["wlat"], m["bw"]
		# Normalizations
		rlat = rlat / 1000 # nsec to usec
		wlat = wlat / 1000
		bw = bw / 1000000 # Kbps to Gbps
		conf = (t, rw, rthreads, threads)
		if chosen_action is action.keep_everything:
			data += [conf + (bw, rlat, wlat)]
			continue
		if conf in samples:
			# samples[conf] = (bw_sum, rlat_sum, wlat_sum, counter)
			if chosen_action is action.keep_mean:
				# If we already have data for the
				# configuration, increase the stored
				# counter and add the recorded bandwidth
				# to the stored aggregate
				(bw_sum, rlat_sum, wlat_sum, counter) = samples[conf]
				bw_sum += bw
				rlat_sum += rlat
				wlat_sum += wlat
				counter += 1
				samples[conf] = (bw_sum, rlat_sum, wlat_sum, counter)
			elif chosen_action is action.keep_last:
				samples[conf] = (bw, rlat, wlat, 1)
		else:
			# If the configuration is seen for
			# the first time, add it to the
			# samples dictionary
			samples[conf] = (bw, rlat, wlat, 1)

	if chosen_action is action.keep_everything:
		return data

	# Take the average for each configuration
	for conf in samples:
		(bw, rlat, wlat, count) = samples[conf]
		normalized_bw = bw / count
		normalized_rlat = rlat / count
		normalized_wlat = wlat / count
		nvalues = (normalized_bw, normalized_rlat, normalized_wlat, )
		conf_with_normalized_value = conf + nvalues
		data += [conf_with_normalized_value]

	return data


def rthreads_to_placement(row):
	remote_threads = row['Remote Threads']
	total_threads = row['Threads']
	if remote_threads == 0:
		return 'Local'
	elif remote_threads == total_threads:
		return 'Remote'
	else:
		return 'Mixed'

def create_graph(data, output_file):
	# Make the plot higher quality
	fig, axes = plt.subplots(nrows=2, ncols=2, figsize=(9,7))
	# plt.suptitle(attrs.title, fontsize=18)

	# Parse the data list to a pandas dataframe
	# (naming the columns as we want)
	df = pd.DataFrame(data, columns =
		['Type', 'Read/Write(%)', 'Remote Threads', 'Threads',
		 'Bandwidth', 'Read Latency', 'Write Latency'])

	# Replace the 'Remote Threads' column with the 'Placement' one
	df['Placement'] = df.apply(
		lambda row : rthreads_to_placement(row), axis=1) 
	df = df.drop('Remote Threads', axis=1)

	access_type = ['Read', 'Write']
	metric = ['Bandwidth', 'Latency']
	
	for row in [0, 1]: # [Read, Write]
		for col in [0, 1]: # [Bandwidth, Latency]
			chosen_rw = 100 if row == 0 else 0
			reduced_df = df[(df['Read/Write(%)'] == chosen_rw)]
			axes[row,col].set_title(
				access_type[row] + ' ' + metric[col],
				fontweight="bold",
				size = 11)
			# What we are plotting
			if col == 0:
				plotted = 'Bandwidth'
			else:
				plotted = access_type[row] + ' Latency'
			
			sns.lineplot(data=reduced_df, x = 'Threads',
				y = plotted, hue = 'Placement',
				palette='Greys', marker='o', dashes=False,
				zorder=3, ax=axes[row,col],
				markeredgecolor='black')
			# Add a grid to the background
			axes[row,col].grid(which='major')
			# Keep the x values integers
			axes[row,col].xaxis.set_major_locator(
				MaxNLocator(integer=True))
			# Set the labels
			xlabel = 'Number of Threads'
			if col == 0:
				ylabel = 'Bandwidth (GB/s)'
			else:
				ylabel = 'Latency (usec)'
			axes[row,col].set(xlabel=xlabel, ylabel=ylabel)
	plt.suptitle("Single PMEM device formatted with Ext4", fontsize=18,
		x = 0.525, fontweight="bold")
	# Layout to prevent overlaps between title, subtitle and the plot
	plt.tight_layout()
	# Make the graph and save it
	plt.show()
	plt.savefig(output_file, dpi=300)


def main():
	if len(sys.argv) != 3:
		print("USAGE: script.py input_file output_file")
		exit

	input_file = sys.argv[1]
	output_file = sys.argv[2]

	data = data_list_from_json(input_file, action.keep_last)
	create_graph(data, re.sub('.png$', '_last.png', output_file))

	data = data_list_from_json(input_file, action.keep_everything)
	create_graph(data, re.sub('.png$', '_all.png', output_file))

if __name__ == "__main__":
    main()
