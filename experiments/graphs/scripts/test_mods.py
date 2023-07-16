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

def data_list_from_json(input_file, chosen_action, normalization_base):
	data = []
	samples = {}
	contents = json.loads(open(input_file, "r").read())
	# Go through each measurement
	for m in contents["measurements"]:
		t, rw = m["type"], m["rw%"]
		rthreads, threads = m["rthreads"], m["threads"]
		bw = m["bw"]
		# Normalizations
		bw = bw / 1000000 # Kbps to Gbps
		assert(rthreads == threads - rthreads)
		conf = (t, str(rw) + "% Read",
			str(rthreads),
			threads)
		if chosen_action is action.keep_everything:
			data += [conf + (bw, rlat, wlat)]
			continue
		if conf in samples:
			if chosen_action is action.keep_mean:
				# If we already have data for the
				# configuration, increase the stored
				# counter and add the recorded bandwidth
				# to the stored aggregate
				(bw_sum, counter) = samples[conf]
				bw_sum += bw
				counter += 1
				samples[conf] = (bw_sum, counter)
			elif chosen_action is action.keep_last:
				samples[conf] = (bw, 1)
		else:
			# If the configuration is seen for
			# the first time, add it to the
			# samples dictionary
			samples[conf] = (bw, 1)

	if chosen_action is action.keep_everything:
		return data

	for conf in samples:
		# Get the same configuration, but
		# for the normalization type
		l = list(conf)
		
		if (l[0] == normalization_base):
			continue
		
		l[0] = normalization_base
		norm_conf = tuple(l)
		
		(norm_bw, norm_count) = samples[norm_conf]
		(bw, count) = samples[conf]		

		normalized_value = (bw * norm_count) / (norm_bw * count)
		conf_with_normalized_value = conf + (normalized_value,)

		data += [conf_with_normalized_value]

	return data

def create_graph(data, attrs, output_file):
	# Parse the data list to a pandas dataframe
	# (naming the columns as we want)
	df = pd.DataFrame(data, columns =
		['Type', 'Read/Write Ratio', 'Thread Placement',
		 'Threads', 'Bandwidth'])
	
	# Make the plot higher quality
	plt.figure(figsize=(9, 7))
	# What we are plotting
	ax = sns.barplot(data=df,
		x='Thread Placement',
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

	# Horizontal line at y = 1
	ax.axhline(1, ls='--', color='r')
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
	title = "EXT4 modifications perfromance gain"
	# Description
	desc  = "256M file for each thread, 4KB IOsize, 3 samples / measurement"
	# Graph attributes
	attrs = graph_attrs(title, desc, '#Threads per Node',
		'Normalized BW')

	data = data_list_from_json(input_file, action.keep_last, 'Linear map (simple)')
	create_graph(data, attrs, output_file)

if __name__=="__main__":
    main()
