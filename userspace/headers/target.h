#define NO_TARGET (-1)
#define DIFF_FACTOR 200000000 // 200 MB

int target_get_initial(struct process *proc);
void fix_congestion(struct process_list *l);
