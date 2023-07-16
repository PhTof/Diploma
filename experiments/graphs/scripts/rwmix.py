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

@dataclass
class graph_attrs:
	title: str
	subtitle: str
	xlabel: str
	ylabel: str

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
		conf = (t, str(rw) + "% Read",
			str(rthreads) + " / " + str(threads), threads)
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

def create_graph(data, attrs, output_file):
	# Parse the data list to a pandas dataframe
	# (naming the columns as we want)
	df = pd.DataFrame(data, columns =
		['Type', 'Read/Write Ratio', 'Remote Threads', 'Threads',
		 'Bandwidth', 'Read Latency', 'Write Latency'])
	# Replace the 'Remote Threads' column with the 'Placement' one
	#df['Placement'] = df.apply(
	#	lambda row : rthreads_to_placement(row), axis=1) 
	#df = df.drop('Remote Threads', axis=1)
	
	# Make the plot higher quality
	plt.figure(figsize=(9, 7))
	# What we are plotting
	ax = sns.barplot(data=df,
		x='Remote Threads',
		y='Bandwidth',
		hue='Read/Write Ratio',
		palette='Greys',
		edgecolor='black',
		zorder=3)
	
	hatches = ['//' , '\\\\' , 'xx']
	for bars, hatch in zip(ax.containers, hatches):
		# Set a different hatch for each group of bars
		for bar in bars:
			bar.set_linewidth(1.5)
			bar.set_hatch(hatch)

	# Add a grid to the background
	ax.grid(which='major')
	# Set the labels
	font = dict(size=12,weight='bold')
	ax.set_xlabel(attrs.xlabel, fontdict=font)
	ax.set_ylabel(attrs.ylabel, fontdict=font)
	# Main title
	fig = ax.get_figure()
	# Requirement for the titles alignment fix
	fig.canvas.draw()
	mid = (fig.subplotpars.right + fig.subplotpars.left) / 2
	plt.suptitle(attrs.title, x=mid, fontsize=18, fontweight="bold",
		y = 0.96)
	# Subtitle
	plt.title(attrs.subtitle, fontsize=10)
	# Layout to prevent overlaps between title, subtitle and the plot
	# plt.tight_layout()
	# Make the graph and save it
	plt.show()
	plt.savefig(output_file, dpi=300)

def main():
	if len(sys.argv) != 3:
		print("USAGE: script.py input_file output_file")
		exit

	input_file = sys.argv[1]
	output_file = sys.argv[2]
	
	# Title
	title = "Mixed Workload with 32 Threads"
	# Description
	desc  = "256M file for each thread, 4KB IOsize, 3 samples / measurement"
	# Graph attributes
	attrs = graph_attrs(title, desc, '# Remote Threads', 'Bandwidth (GB/s)')

	data = data_list_from_json(input_file, action.keep_mean)
	create_graph(data, attrs, output_file)

if __name__=="__main__":
    main()
