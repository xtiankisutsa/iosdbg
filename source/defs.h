/*
Hold important definitions for things being used everywhere.
*/

#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <mach/mach.h>
#include <mach/arm/exception.h>
#include <pthread/pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "printutils.h" // rl_printf

#include <armadillo.h>

static const char *prompt = "(iosdbg) ";

// General errors.
typedef int gen_error_t;

#define GEN_SUCCESS (gen_error_t)0
#define GEN_FAILURE (gen_error_t)1

#define MAX_EXCEPTION_PORTS 16

struct original_exception_ports_t {
	mach_msg_type_number_t count;
	exception_mask_t masks[MAX_EXCEPTION_PORTS];
	exception_handler_t ports[MAX_EXCEPTION_PORTS];
	exception_behavior_t behaviors[MAX_EXCEPTION_PORTS];
	thread_state_flavor_t flavors[MAX_EXCEPTION_PORTS];
};

struct debuggee {
	// Task port to the debuggee.
	mach_port_t task;

	// PID of the debuggee.
	pid_t pid;

	// Port to notify us upon debuggee's termination.
	mach_port_t death_port;

	// Whether execution has been suspended or not.
	int interrupted;

	// Keeps track of the ID of the last breakpoint that hit.
	int last_hit_bkpt_ID;

	/* Keeps track of the type of the last breakpoint hit. */
	int last_hit_bkpt_hw;

	/* Keeps track of the location of the data in the last watchpoint hit. */
	unsigned long last_hit_wp_loc;

	/* Keeps track of where the last watchpoint hit. */
	unsigned long last_hit_wp_PC;

	// How many breakpoints are set.
	int num_breakpoints;

	// How many watchpoints are set.
	int num_watchpoints;

	// The debuggee's name.
	char *debuggee_name;

	// How many hardware breakpoints the device supports.
	int num_hw_bps;

	// How many hardware watchpoints the device supports.
	int num_hw_wps;

	// Thread state for the debuggee.
	arm_thread_state64_t thread_state;

	// Debug state for the debuggee.
	arm_debug_state64_t debug_state;

	// Count of threads for the debuggee.
	mach_msg_type_number_t thread_count;

	// Port to get exceptions from the debuggee.
	mach_port_t exception_port;

	// Saved exception ports from the debuggee.
	struct original_exception_ports_t original_exception_ports;

	// List of breakpoints on the debuggee.
	struct linkedlist *breakpoints;

	// List of watchpoints on the debuggee.
	struct linkedlist *watchpoints;

	// List of threads on the debuggee.
	struct linkedlist *threads;

	// The debuggee's ASLR slide.
	unsigned long long aslr_slide;

	// The function pointer to find the debuggee's ASLR slide
	unsigned long long (*find_slide)(void);

	// The function pointer to restore original exception ports
	kern_return_t (*restore_exception_ports)(void);

	// The function pointer to task_resume
	kern_return_t (*resume)(void);

	// The function pointer to set up exception handling
	kern_return_t (*setup_exception_handling)(void);

	// The function pointer to task_suspend
	kern_return_t (*suspend)(void);

	// The function pointer to update the list of the debuggee's threads
	kern_return_t (*update_threads)(thread_act_port_array_t *);

	// The function pointer to get the debug thread state of the debuggee's focused thread.
	kern_return_t (*get_debug_state)(void);

	// The function pointer to set the debug thread state of the debuggee's focused thread.
	kern_return_t (*set_debug_state)(void);
	
	// The function pointer to get the thread state of the debuggee's focused thread.
	kern_return_t (*get_thread_state)(void);

	// The function pointer to set the thread state of the debuggee's focused thread.
	kern_return_t (*set_thread_state)(void);
};

struct debuggee *debuggee;

#endif