/**
 * CS 240 Shell Spells
 * Ben Wood, 2015
 *
 * Terminal and signals management
 *
 * These functions handle interaction with the terminal interface to
 * ensure keyboard interrupts (like Ctrl-C and Ctrl-Z) and other
 * signals are properly delivered to foreground processes in the
 * shell.
 */
#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include "joblist.h"

/* Setup terminal and signals for shell. 
 * Call once to initialize jobs when shell starts. */
void term_shell_init(JobList* jobs);

/* Setup terminal and signals in a child process.  Call once in each
 * child process before exec-ing to "move" it into the foreground.
 * (Must also call term_give in parent.) */
void term_child_init(JobList* jobs, int foreground);

/* Attach terminal signals to given job.  Call once in shell process
 * to "move" the given job into the foreground. (Must also call
 * term_child_init in child.) */
void term_give(JobList* jobs, Job* new_foreground_job);

/* Re-attach terminal signals back to shell.  Call after a foreground
 * job has left the foreground. */
void term_take(JobList* jobs, Job* previous_foreground_job);



#endif // _TERMINAL_H_
