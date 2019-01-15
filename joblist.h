/**
 * CS 240 Shell Spells
 * Ben Wood, 2015
 *
 * Job list management
 *
 * These data structures and functions provide management of lists of
 * jobs in the shell.
 */
#ifndef _JOBLIST_H_
#define _JOBLIST_H_

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>


/********** DATA TYPES **********/

/**
 * Job ID type.
 */
typedef int Jid;

/**
 * Start Job IDs at 1
 */
#define JOBLIST_FIRST_JID 1
#define JOBLIST_NO_JID 0

/**
 * Job status
 */
// No status (invalid status) -- should appear only transiently before new job is fully initialized.
#define JOB_STATUS_NONE       0
// Job was running in foreground last we knew
#define JOB_STATUS_FOREGROUND 1
// Job was stopped last we knew
#define JOB_STATUS_STOPPED    2
// Job was running in background last we knew
#define JOB_STATUS_BACKGROUND 3
// Job is done
#define JOB_STATUS_DONE       4
typedef int JobStatus;
  
/**
 * Job record.  
 * Read first four fields directly.
 * Allocate, modify, and free only via functions below.
 */
typedef struct Job {
  // OK to read these directly.

  /* Job ID of this job. */
  Jid jid;
  /* Prcoess ID of this job's process. */
  pid_t pid;
  /* exec-format command array */
  char** command;
  /* Is the process stopped or running? */
  JobStatus status;

  // BOOKKEEPING -- don't touch these.

  /* Link to next node in list of jobs. */
  struct Job* next;
  struct Job* prev;
  /* Terminal mode bookkeeping */
  struct termios tmodes;
} Job;

/**
 * Job list.
 * Allocate, access, and free only via functions below.
 */
typedef struct JobList {
  /* Reference to head of list. */
  Job* head;
  /* Reference to tail of list. */
  Job* tail;
  /* ID of "current" job -- job most recently started in background or
     stopped in the foreground. */
  Job* current;

  // Extras -- do not need to understand.
  /* Shell's process group ID. */
  pid_t pgid;
  /* Terminal ID */
  int term;
  int interactive;
  /* Terminal mode bookkeeping */
  struct termios tmodes;
} JobList;



/********** OPERATIONS **********/


/* Allocate and initialize a fresh joblist. */
JobList* joblist_create();

/* Check if a job list is empty */
int joblist_empty(JobList* jobs);

/* Free a (presumed empty) job list. */
void joblist_free(JobList* jobs);

/* Save a new job given pid, command, and status.  The new job receves
 * the next available job ID.  The command array is saved (not copied)
 * as part of the job data structure.  DO NOT FREE command explicitly
 * after calling job_save with it.*/
Job* job_save(JobList* jobs, pid_t pid, char** command, JobStatus status);

/* Get the job with Job ID jid. */
Job* job_get(JobList* jobs, Jid jid);

/* Get the "current" Job if any.  The "current" job is the
 * most-recently touched non-foreground job. */
Job* job_get_current(JobList* jobs);

/* Print one job, including, job ID, "current"-ness, PID,
 * exectuion status, and command. */
void job_print(JobList* jobs, Job* job);

/* Make a state transition. */
void job_set_status(JobList* jobs, Job* job, JobStatus status);

/* Remove the given job from the list and free it, INCLUDING ITS COMMAND. */
void job_delete(JobList* jobs, Job* job);

/* Call function fp once on each job in jobs.  */
void job_iter(JobList* jobs, void (*fp)(JobList*,Job*));



#endif // _JOBLIST_H_
