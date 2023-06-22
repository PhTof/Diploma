#include <linux/printk.h>

#include "ext4.h"
#include "numa.h"

#define EXT4_NUMA_DEBUG 0

/*
 * NUMA NODE i (memory of size "mapped_size_bytes")
 * +-------------------+-------+-----+-------+-----------------+
 * | unavailable_bytes | group | ... | group | remaining_bytes |
 * +-------------------+-------+-----+-------+-----------------+
 *
 * unavailable_bytes = bytes that don't belong to the first 
 * 	group of this node
 * remaining_bytes = bytes that don't fit in a whole group 
 * 	in the memory of this node
 * */

void ext4_numa_super_init(struct ext4_sb_info *sbi)
{
	int i, num_nodes = EXT4_NUMA_NUM_NODES;
	// Typically this should be of type ext4_group_t (unsigned)
	// but using do_div requires the divident (measured in bytes)
	// to be stored first in this variable
	u64 groups_in_this_node;
	ext4_group_t groups_counted_so_far = 0;
	struct super_block *sb = sbi->s_sb;
	u64 mapped_size_bytes;
	unsigned int blocksize_bits = sb->s_blocksize_bits;
	// Divier of do_div (bytes_in_group) of do_div must be a 32 bit integer
	// (a u32 is barely enough if we want to extend this to work with flex
	// groups, since for "Flex block group size" = 16, we get 16 times
	// 2^15 blocks per group times 2^12 bytes per block = 2^31)
	// BETTER IDEA: use div64_u64 from linux/math64.h maybe
	// EVEN BETTER IDEA: Most possibly, everything is a power of 2 anyways, so
	// just store the shift offsets and avoid divisions and mutliplications 
	// altogether
	u32 unavailable_bytes = sbi->s_es->s_first_data_block << blocksize_bits;
	u32 remaining_bytes;
	u32 bytes_in_group = (sbi->s_blocks_per_group) << blocksize_bits;
	bool shared_group;

	mapped_size_bytes = sb->s_bdev->bd_inode->i_size;
	do_div(mapped_size_bytes, num_nodes);

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
		 * -> EVEN MORE IMPROVED RULE: use the first rule and
		 * the device mapper wisely to avoid having shared 
		 * groups by trimming a bit the size used of each device
		 * */
		groups_in_this_node = (mapped_size_bytes - unavailable_bytes);
		remaining_bytes = do_div(groups_in_this_node, bytes_in_group);
		shared_group = (remaining_bytes > 0);
		sbi->s_numa_info.first_group[i] = groups_counted_so_far;
		sbi->s_numa_info.total_groups[i] = groups_in_this_node + shared_group;
		groups_counted_so_far += groups_in_this_node + shared_group;
		unavailable_bytes = shared_group ? (bytes_in_group - remaining_bytes) : 0;
	}

	for(i = 0; i < num_nodes; i++) {
		sbi->s_mb_last_group[i] = sbi->s_numa_info.first_group[i];
		// I think this is correct
		sbi->s_mb_last_start[i] = 0;
	}

	#if (EXT4_NUMA_DEBUG == 1)
	printk(KERN_INFO "==== SUPER NUMA DEBUG INFO ====\n");
	for(i = 0; i < num_nodes; i++) {
		printk(KERN_INFO "node = %d : (first group) %u | (total_groups) %u \n",
			i, sbi->s_numa_info.first_group[i], sbi->s_numa_info.total_groups[i]);
	}
	printk(KERN_INFO "===============================\n");
	#endif

	sbi->s_numa_info.num_nodes = num_nodes;
}
