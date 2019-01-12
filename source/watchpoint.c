#include "watchpoint.h"

/* Find an available hardware watchpoint register.*/
int find_ready_wp_reg(void){
	struct node_t *current = debuggee->watchpoints->front;
	
	/* Keep track of what hardware watchpoint registers are used
	 * in the watchpoints currently active.
	 */
	int *wp_map = malloc(sizeof(int) * debuggee->num_hw_wps);

	/* -1 means the hardware watchpoint register representing that spot
	 * in the array has not been used. 0 means the opposite.
	 */
	memset(wp_map, (int)-1, debuggee->num_hw_wps);

	while(current){
		struct watchpoint *current_watchpoint = (struct watchpoint *)current->data;

		wp_map[current_watchpoint->hw_wp_reg] = 0;

		current = current->next;
	}

	/* Now search wp_map for any empty spots. */
	for(int i=0; i<debuggee->num_hw_wps; i++){
		if(wp_map[i] != 0){
			free(wp_map);
			return i;
		}
	}

	free(wp_map);

	/* No available hardware watchpoint registers found. */
	return -1;
}

struct watchpoint *watchpoint_new(unsigned long location, unsigned int data_len, int LSC){
	if(data_len == 0 || data_len > sizeof(unsigned long))
		return NULL;

	kern_return_t result = memutils_valid_location(location);
	
	if(result)
		return NULL;
	
	struct watchpoint *wp = malloc(sizeof(struct watchpoint));
	
	wp->location = location;
	wp->hit_count = 0;
	
	wp->data_len = data_len;
	wp->data = malloc(wp->data_len);

	result = memutils_read_memory_at_location((void *)wp->location, wp->data, wp->data_len);

	if(result){
		free(wp);
		return NULL;
	}

	int available_wp_reg = find_ready_wp_reg();

	if(available_wp_reg == -1){
		free(wp);
		return NULL;
	}

	debuggee->get_debug_state();

	/* Setup the DBGWCR<n>_EL1 register.
	 * We need the following criteria to correctly set up this watchpoint:
	 *  - WT must be 0, we hare doing an unlinked data match.
	 *  - BAS must be <data_len> active bits.
	 *  - LSC varies on what arguments the user provides.
	 *  - PAC must be 0b10 so these watchpoints generate debug events in EL0,
	 *    where we are executing.
	 *  - E must be 0b1 so this watchpoint is enabled.
	 */
	int BAS_ = (((1 << wp->data_len) - 1) << 5);
	LSC <<= 3;

	wp->LSC = LSC;

	debuggee->debug_state.__wcr[available_wp_reg] = 
		WT |
		BAS_ |
		LSC |
		PAC |
		E;

	/* Bits[1:0] must be clear in DBGWVR<n>_EL1. */
	location &= ~0x3;

	/* Put the location in whichever DBGWVR<n>_EL1 is available. */
	debuggee->debug_state.__wvr[available_wp_reg] = location;

	debuggee->set_debug_state();

	wp->hw_wp_reg = available_wp_reg;
	wp->id = current_watchpoint_id++;

	return wp;
}

wp_error_t watchpoint_at_address(unsigned long location, unsigned int data_len, int LSC){
	struct watchpoint *wp = watchpoint_new(location, data_len, LSC);

	if(!wp){
		printf("Could not set watchpoint\n");
		return WP_FAILURE;
	}
	
	linkedlist_add(debuggee->watchpoints, wp);

	const char *type = "r";

	if(LSC == WP_WRITE)
		type = "w";
	else if(LSC == WP_READ_WRITE)
		type = "rw";

	printf("Watchpoint %d: addr = %#lx size = %d type = %s\n", wp->id, wp->location, wp->data_len, type);
	
	debuggee->num_watchpoints++;

	return WP_SUCCESS;
}

void watchpoint_hit(struct watchpoint *wp){
	if(!wp)
		return;

	wp->hit_count++;
}

void enable_wp(struct watchpoint *wp){
	debuggee->get_debug_state();
	
	int BAS_ = (((1 << wp->data_len) - 1) << 5);
	
	debuggee->debug_state.__wcr[wp->hw_wp_reg] =
		WT |
		BAS_ |
		wp->LSC |
		PAC |
		E;

	debuggee->debug_state.__wvr[wp->hw_wp_reg] = (wp->location & ~0x3);

	debuggee->set_debug_state();
}

void disable_wp(struct watchpoint *wp){
	debuggee->get_debug_state();
	debuggee->debug_state.__wcr[wp->hw_wp_reg] = 0;
	debuggee->set_debug_state();
}

void wp_set_state_internal(struct watchpoint *wp, int disabled){
	if(disabled)
		disable_wp(wp);
	else
		enable_wp(wp);
}

void wp_delete_internal(struct watchpoint *wp){
	disable_wp(wp);
	linkedlist_delete(debuggee->watchpoints, wp);

	debuggee->num_watchpoints--;
}

wp_error_t watchpoint_delete(int wp_id){
	if(!debuggee->watchpoints->front)
		return WP_FAILURE;

	if(wp_id == 0)
		return WP_FAILURE;

	struct node_t *current = debuggee->watchpoints->front;

	while(current){
		struct watchpoint *current_watchpoint = (struct watchpoint *)current->data;

		if(current_watchpoint->id == wp_id){		
			wp_delete_internal(current_watchpoint);
			return WP_SUCCESS;
		}

		current = current->next;
	}

	return WP_FAILURE;
}

void watchpoint_enable_all(void){
	if(!debuggee->watchpoints->front)
		return;
	
	struct node_t *current = debuggee->watchpoints->front;

	while(current){
		struct watchpoint *current_watchpoint = (struct watchpoint *)current->data;

		wp_set_state_internal(current_watchpoint, WP_ENABLED);

		current = current->next;
	}
}

void watchpoint_disable_all(void){
	if(!debuggee->watchpoints->front)
		return;
	
	struct node_t *current = debuggee->watchpoints->front;

	while(current){
		struct watchpoint *current_watchpoint = (struct watchpoint *)current->data;
		
		wp_set_state_internal(current_watchpoint, WP_DISABLED);

		current = current->next;
	}
}

void watchpoint_delete_all(void){
	if(!debuggee->watchpoints->front)
		return;

	struct node_t *current = debuggee->watchpoints->front;

	while(current){
		struct watchpoint *current_watchpoint = (struct watchpoint *)current->data;
		
		wp_delete_internal(current_watchpoint);
		
		current = current->next;
	}
}

struct watchpoint *find_wp_with_address(unsigned long addr){
	if(!debuggee->watchpoints->front)
		return NULL;

	struct node_t *current = debuggee->watchpoints->front;

	while(current){
		struct watchpoint *current_watchpoint = (struct watchpoint *)current->data;

		if(current_watchpoint->location == addr)
			return current_watchpoint;

		current = current->next;
	}
	
	return NULL;
}