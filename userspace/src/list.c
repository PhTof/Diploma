#include <stdbool.h>
#include "../headers/processes.h"

bool list_is_empty(struct process_list *l) {
	return (l->size == 0);
}
