#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pthread/pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
//#include "linenoise.h"
#include "mach_exc.h"
#include "linkedlist.h"
#include "breakpoint.h"
#include "memutils.h"
#include "matchcmd.h"
#include "defs.h"

// Implemented in iosdbg.c
void help();
const char *get_exception_code(exception_type_t);
void setup_initial_debuggee();
int resume();
int detach();
void resume_threads();
int suspend_threads();
int attach(pid_t);
void interrupt(int);
int show_general_registers(int);
int show_neon_register(char, int);
void setup_exception_handling();
int set_breakpoint(unsigned long long);
int delete_breakpoint(int);
pid_t pid_of_program(char *);