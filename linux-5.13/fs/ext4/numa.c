#include "ext4.h"
/* ADDITION */
#include <linux/printk.h>

int ext4_numa_bg_node(struct super_block *sb, ext4_group_t g)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	int num_nodes = sbi->s_numa_info.num_nodes;
	ext4_group_t ngroups = sbi->s_groups_count;
	ext4_group_t *first = sbi->s_numa_info.first_group;
	ext4_group_t *total = sbi->s_numa_info.total_groups;
	int node;

	// It is very probable that the group number will be
	// something out of bounds
	g = g % ngroups;

	for(node = 0; node < num_nodes; node++)
		if(first[node] <= g && g < first[node] + total[node])
			return node;

	return 0;
}


ext4_group_t ext4_numa_map_any_block(
	struct super_block *sb, ext4_group_t group, int map_node)
{	
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_numa_info *nf = &(sbi->s_numa_info);
	ext4_group_t ngroups = sbi->s_groups_count;
	ext4_group_t *first = nf->first_group;
	ext4_group_t total_groups = nf->total_groups[map_node];
	int block_node;

	group = group % ngroups;
	block_node = ext4_numa_bg_node(sb, group);

	return first[map_node] + (group - first[block_node]) % total_groups;
}

void ext4_numa_super_init(struct ext4_sb_info *sbi)
{
	int i, num_nodes = EXT4_NUMA_NUM_NODES;
	int plus_idx, plus_count = 0;
	ext4_group_t ngroups = sbi->s_groups_count;
	ext4_group_t groups_per_node, groups_in_this_node;
	ext4_group_t groups_counted_so_far = 0, remaining;

	groups_per_node = ngroups / num_nodes;
	remaining = ngroups % num_nodes;

	// If the number of groups is divided perfectly,
	// then plus_idx is such that no node gets an
	// additional group
	plus_idx = (remaining > 0) ? 0 : (-1);

	for(i = 0; i < num_nodes; i++) {
		/* -> RULE (FOR NOW): Each shared group belongs to the
		 * numa node with the smallest index
		 * -> IMPROVED RULE (TO BE IMPLEMENTED LATER): each
		 * shared group belongs to no numa node (don't let
		 * data sharing destroy performance)
		 * PROBLEM WITH THIS: During mutliblock allocation,
		 * it is probable that after a failed call to ext4_
		 * mb_regular_allocator (during which we ignored 
		 * any shared groups), it will be checked that there
		 * are adequate free blocks in the shared groups,
		 * resulting in infinite retries (maybe).
		 * */
		/* It can be proved that, for the first rule, we have:
		 * - Every node has either round_down(ngroups / num_nodes)
		 *   groups or round_down(ngroups / num_nodes) + 1 groups
		 * - The latter nodes have indices idx s.t. idx = 0 or
		 *   idx = round_up((i*num_nodes) / remainder) - 1 for
		 *   i = 1 ... (remainder - 1) and remainder = ngroups % 
		 *   num_nodes
		 * */
		groups_in_this_node = groups_per_node;
		if (i == plus_idx && plus_idx < num_nodes - 1) {
			groups_in_this_node++;
			plus_idx = roundup((++plus_count)*num_nodes, remaining) - 1;
		}
		sbi->s_numa_info.first_group[i] = groups_counted_so_far;
		sbi->s_numa_info.total_groups[i] = groups_in_this_node;
		groups_counted_so_far += groups_in_this_node;
	}

	for(i = 0; i < num_nodes; i++) {
		sbi->s_mb_last_group[i] = sbi->s_numa_info.first_group[i];
		// I think this is correct
		sbi->s_mb_last_start[i] = 0;
	}
	
	sbi->s_numa_info.num_nodes = num_nodes;
}
