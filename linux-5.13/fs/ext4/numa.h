#ifndef _EXT4_NUMA_H
#define _EXT4_NUMA_H

#define EXT4_NUMA_NUM_NODES 2

// I finally understood that definitions like this
// help a lot when it is difficult to just include
// the stuff you need (in this case, I can not just
// do #include "ext4.h", because the definition of 
// ext4_sb_info depends on this header file)
#define ext4_numa_map_block(sb, group, node) \
	ext4_numa_map_block_fast(&(EXT4_SB(sb)->s_numa_info), group, node);

#define for_each_numa_node(_n, var_node, init_node, num_nodes) 	\
	for (_n = 0, var_node = init_node; 			\
	     _n < num_nodes; 					\
	     _n++, var_node = (var_node + 1) % num_nodes)

struct ext4_numa_info {
	int num_nodes;
	ext4_group_t first_group[EXT4_NUMA_NUM_NODES];
	ext4_group_t total_groups[EXT4_NUMA_NUM_NODES];
};

extern int ext4_numa_bg_node(struct super_block *sb, ext4_group_t g);
extern ext4_group_t ext4_numa_map_any_block(
	struct super_block *sb, ext4_group_t group, int map_node);
extern void ext4_numa_super_init(struct ext4_sb_info *sbi);

// We are mainly concerned with the case when 
// group belongs to node or (node + 1), to achieve 
// circular traversal of the blocks
static inline ext4_group_t ext4_numa_map_block_fast(
	struct ext4_numa_info *nf, ext4_group_t group, int map_node)
{	
	ext4_group_t first_group = nf->first_group[map_node];
	ext4_group_t total_groups = nf->total_groups[map_node];

	return first_group + (group - first_group) % total_groups;
}

#endif
