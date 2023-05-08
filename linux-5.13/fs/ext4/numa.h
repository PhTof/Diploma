#ifndef _EXT4_NUMA_H
#define _EXT4_NUMA_H

#include <linux/topology.h>
#define EXT4_NUMA_NUM_NODES 2

// I finally understood that definitions like this
// help a lot when it is difficult to just include
// the stuff you need (in this case, I can not just
// do #include "ext4.h", because the definition of 
// ext4_sb_info depends on this header file)
#define ext4_numa_map_block(sb, group, node, ngroups) ({ 	\
	struct ext4_sb_info *sbi = EXT4_SB(sb); 		\
	ext4_numa_map_block_fast( 				\
		&(sbi->s_numa_info), 				\
		group, node, ngroups); 				\
	})

#define for_each_numa_node(_n, var_node, init_node, num_nodes) 	\
	for (_n = 0, var_node = init_node; 			\
	     _n < num_nodes; 					\
	     _n++, var_node = (var_node + 1) % num_nodes)

struct ext4_numa_info {
	int num_nodes;
	ext4_group_t first_group[EXT4_NUMA_NUM_NODES];
	ext4_group_t total_groups[EXT4_NUMA_NUM_NODES];
};

extern bool ext4_numa_enabled(void);
extern int ext4_numa_bg_node(struct super_block *sb, ext4_group_t g);
extern int ext4_numa_num_nodes(struct super_block *sb);
extern ext4_group_t ext4_numa_num_groups(
	struct super_block *sb, int node);
extern ext4_group_t ext4_numa_map_any_block(
	struct super_block *sb, ext4_group_t group, int map_node);
extern void ext4_numa_super_init(struct ext4_sb_info *sbi);

static inline int ext4_numa_node_id(void) {
	return ext4_numa_enabled() ? numa_node_id() : 0;
}

// We are mainly concerned with the case when 
// group belongs to node or (node + 1), to achieve 
// circular traversal of the blocks
static inline ext4_group_t ext4_numa_map_block_fast(
	struct ext4_numa_info *nf, ext4_group_t group, 
	int map_node, ext4_group_t total_groups)
{	
	ext4_group_t first_group = nf->first_group[map_node];
	ext4_group_t n_node_groups = nf->total_groups[map_node];

	if (!ext4_numa_enabled())
		return group % total_groups;

	return first_group + (group - first_group) % n_node_groups;
}

static inline int ext4_numa_next_node(int node, int total_nodes) {
	return (node + 1) % total_nodes;
}

#endif
