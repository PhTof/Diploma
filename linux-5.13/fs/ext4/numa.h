#ifndef _EXT4_NUMA_H
#define _EXT4_NUMA_H

#include <linux/topology.h>

#include "ext4.h"

#define for_each_numa_node(_n, var_node, init_node, num_nodes) 	\
	for (_n = 0, var_node = init_node; 			\
	     _n < num_nodes; 					\
	     _n++, var_node = (var_node + 1) % num_nodes)

static inline bool ext4_numa_enabled(struct super_block *sb) {
	return test_opt2(sb, NUMA);
}

static inline int ext4_numa_node_id(struct super_block *sb) {
	return ext4_numa_enabled(sb) ? numa_node_id() : 0;
}

static inline int ext4_numa_next_node(int node) {
	return (node + 1) % EXT4_NUMA_NUM_NODES;
}

static inline int ext4_numa_num_nodes(struct super_block *sb) {
	int num_nodes = EXT4_SB(sb)->s_numa_info.num_nodes;
	return ext4_numa_enabled(sb) ? num_nodes : 1;
}

static inline int ext4_numa_bg_node(struct super_block *sb, ext4_group_t g)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	int num_nodes = sbi->s_numa_info.num_nodes;
	ext4_group_t *first = sbi->s_numa_info.first_group;
	ext4_group_t *total = sbi->s_numa_info.total_groups;
	int node;

	if(!ext4_numa_enabled(sb))
		return 0;

	for(node = 0; node < num_nodes; node++)
		if(first[node] <= g && g < first[node] + total[node])
			return node;

	return 0;
}

/* Here, we are only concerned with the case when group 
 * belongs to a node that is >= map_node, to achieve 
 * circular traversal of the blocks */
static inline ext4_group_t ext4_numa_map_group(struct super_block *sb,
		ext4_group_t group, int map_node, ext4_group_t ngroups)
{
	struct ext4_numa_info *nf = &EXT4_SB(sb)->s_numa_info;
	ext4_group_t first_group = nf->first_group[map_node];
	ext4_group_t n_node_groups = nf->total_groups[map_node];
	
	group = group % ngroups;

	if (!ext4_numa_enabled(sb))
		return group;

	return first_group + (group - first_group) % n_node_groups;
}

static inline ext4_group_t ext4_numa_map_any_group(struct super_block *sb,
		ext4_group_t group, int map_node)
{	
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_numa_info *nf = &(sbi->s_numa_info);
	ext4_group_t ngroups = sbi->s_groups_count;
	ext4_group_t *first = nf->first_group;
	ext4_group_t total_groups = nf->total_groups[map_node];
	int group_node;
	
	group = group % ngroups;
	
	if (!ext4_numa_enabled(sb))
		return group;

	group_node = ext4_numa_bg_node(sb, group);

	return first[map_node] + (group - first[group_node]) % total_groups;
}


static inline ext4_fsblk_t ext4_numa_map_any_block(struct super_block *sb,
		ext4_fsblk_t block, int map_node)
{	
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_numa_info *nf = &(sbi->s_numa_info);
	ext4_grpblk_t blks_per_grp = EXT4_BLOCKS_PER_GROUP(sb);
	ext4_group_t *first = nf->first_group;
	ext4_group_t group_of_block;
	ext4_fsblk_t blk_node_first_block, map_node_first_block;
	int blk_node;
	
	if (!ext4_numa_enabled(sb))
		return block;

	group_of_block = ext4_get_group_number(sb, block);
	blk_node = ext4_numa_bg_node(sb, group_of_block);
	blk_node_first_block = (ext4_fsblk_t) first[blk_node] * blks_per_grp;
	map_node_first_block = (ext4_fsblk_t) first[map_node] * blks_per_grp;

	return map_node_first_block + (block - blk_node_first_block);
}

static inline int ext4_numa_block_node(struct super_block *sb,
		ext4_fsblk_t blk)
{
	ext4_group_t group = ext4_get_group_number(sb, blk);
	return ext4_numa_bg_node(sb, group);
}

static inline ext4_group_t ext4_numa_num_groups(
		struct super_block *sb, int node) 
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);

	if (!ext4_numa_enabled(sb))
		return sbi->s_groups_count;
	
	return sbi->s_numa_info.total_groups[node];
}

static inline ext4_group_t ext4_numa_first_group(
		struct super_block *sb, int node) 
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);

	if (!ext4_numa_enabled(sb))
		return 0;
	
	return sbi->s_numa_info.first_group[node];
}

extern void ext4_numa_super_init(struct ext4_sb_info *sbi);

#endif
