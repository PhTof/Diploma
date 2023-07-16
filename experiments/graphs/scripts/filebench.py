import json
import sys
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib as mpl

def get_data(filename):
	samples = {}
	data = []

	with open(filename, 'r') as f:
		contents = json.loads(f.read())

	for measurement in contents["measurements"]:
		workload = measurement['workload']
		configuration = measurement['configuration']
		ops = measurement['ops bw']
		key = (workload, configuration)
		samples[key] = ops

	for key in samples:
		norm_key = (key[0], 'Single device')
		
		ops = samples[key]
		norm_ops = samples[norm_key]		
		
		data += [key + (ops/norm_ops,)]

	return data


def create_graph(data, output_graph):
	df = pd.DataFrame(data, columns =
		['Workload', 'Configuration', 'Normalized Ops'])
	
	plt.figure(figsize=(9, 7))
	ax = sns.barplot(data=df,
		x='Workload',
		y='Normalized Ops',
		hue='Configuration',
		palette='Greys',
		edgecolor='black',
		zorder=3)
	
	# mpl.rcParams['hatch.linewidth'] = 3.0
	hatches = ['//' , '\\\\' , '--' , 'xx']
	for bars, hatch in zip(ax.containers, hatches):
		# Set a different hatch for each group of bars
		for bar in bars:
			bar.set_linewidth(1.5)
			bar.set_hatch(hatch)
	
	ax.axhline(1, ls='--', color='b')
	ax.grid(which='major')
	font = dict(size=12,weight='bold')
	ax.set_xlabel('Workload', fontdict=font)
	ax.set_ylabel('Normalized Ops', fontdict=font)
	plt.suptitle("Evaluation with Filebench", fontsize=18, fontweight="bold")
	plt.tight_layout()
	plt.show()
	plt.savefig(output_graph, dpi=300)


def main():
	filename = sys.argv[1]
	output_graph = sys.argv[2]
	create_graph(get_data(filename), output_graph)
	

if __name__=="__main__":
    main()
