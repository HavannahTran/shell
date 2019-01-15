/**
 * CS 240 Shell Spells
 * Ben Wood, 2015
 *
 * Job list management
 *
 * See joblist.h for official documentation and usage.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>

#include "joblist.h"
#include "command.h"

/**
 * Allocate and initialize a new JobList.
 */
JobList* joblist_create() {
  JobList* jobs = malloc(sizeof(JobList));
  assert(jobs);
  jobs->head = NULL;
  jobs->tail = NULL;
  jobs->current = NULL;
  return jobs;
}

/**
 * Check if list is empty.
 */
int joblist_empty(JobList* jobs) {
  return ! jobs->head;
}

/**
 * Free a JobList, assumed to be empty.
 * (Any contained Jobs are not freed.)
 */
void joblist_free(JobList* jobs) {
  assert(jobs);
  assert(!jobs->head && !jobs->tail && !jobs->current);
  free(jobs);
}

/**
 * Free a Job, including the command it carries.
 */
void job_free(Job* job) {
  assert(job);
  command_free(job->command);
  free(job);
}

/**
 * Create and save a new Job in the given JobList,
 * with the given pid, command, and initial status.
 */
Job* job_save(JobList* jobs,
               pid_t pid,  char** command, JobStatus status) {
  assert(jobs);
  assert(pid > 0);
  assert(command);
  Job* job = (Job*)malloc(sizeof(Job));
  assert(job);
  job->next = NULL;
  job->pid = pid;
  job->command = command;
  job->status = status;
  // Insert at end (or as head and tail if none exist.)
  if (jobs->tail) {
    jobs->tail->next = job;
    // Initial Job ID is max Job ID + 1.
    job->jid = jobs->tail->jid + 1;
  } else {
    jobs->head = job;
    // Initial Job ID is the first ID.
    job->jid = JOBLIST_FIRST_JID;
  }
  job->prev = jobs->tail;
  jobs->tail = job;

  // If background or stopped, set as current.
  if (status == JOB_STATUS_BACKGROUND
      || status == JOB_STATUS_STOPPED) {
    jobs->current = job;
  }

  return job;
}

/**
 * Set the job's execution status and update the current job
 * if applicable.
 */
void job_set_status(JobList* jobs, Job* job, JobStatus status) {
  assert(jobs);
  assert(job);
  if (status == JOB_STATUS_BACKGROUND
      || status == JOB_STATUS_STOPPED) {
    jobs->current = job;
  } else if (jobs->current == job) {
    jobs->current = NULL;
  }
  job->status = status;
}

/**
 * Print a job.
 */
void job_print(JobList* jobs, Job* job) {
  assert(jobs);
  assert(job);
  char* status;
  int bg = 0;
  switch (job->status) {
  case JOB_STATUS_NONE:
    status = "INVALID";
    break;
  case JOB_STATUS_FOREGROUND:
    status = "Foreground";
    break;
  case JOB_STATUS_STOPPED:
    status = "Stopped";
    break;
  case JOB_STATUS_BACKGROUND:
    status = "Running";
    bg = 1;
    break;
  case JOB_STATUS_DONE:
    status = "Done   ";
    bg = 1;
    break;
  }
  printf("[%d]%s %d  %s    ",
         job->jid,
         (jobs->current == job ? "+" : " "), 
         job->pid,
         status);
  command_print(job->command);
  if (bg) printf("&");
  printf("\n");
}

/** Get a pointer to the Job with ID jid. */
Job* job_get(JobList* jobs, Jid jid) {
  assert(jobs);
  assert(jid >= JOBLIST_FIRST_JID);
  for (Job* job = jobs->head; job; job = job->next) {
    if (job->jid == jid) return job;
  }
  return NULL;
}

/** Get pointer to "current" job. */
Job* job_get_current(JobList* jobs) {
  assert(jobs);
  return jobs->current;
}

/** 
 * Remove the given finished Job from the job list and free it.
 */
void job_delete(JobList* jobs, Job* job) {
  assert(jobs);
  assert(job);

  // If it is the head, there is a new head.
  if (jobs->head == job) {
    jobs->head = job->next;
  }

  // If it is the tail, there is a new tail.
  if (jobs->tail == job) {
    jobs->tail = job->prev;
  }

  // If it is the current job, there is no longer a current job.
  if (jobs->current == job) {
    jobs->current = JOBLIST_NO_JID;
  }

  // Splice it out of the list.
  if (job->prev) {
    job->prev->next = job->next;
  }
  if (job->next) {
    job->next->prev = job->prev;
  }

  // For sanity, null its link pointers.
  job->prev = NULL;
  job->next = NULL;

  // Free.
  job_free(job);
}

/** 
 * Iterate over joblist, calling fp once on each job. 
 * fp is a pointer to a function that takes two arguments:
 * a Joblist* and a Job*
*/
void job_iter(JobList* jobs, void (*fp)(JobList*, Job*)) {
  assert(jobs);
  assert(fp);
  for (Job* job = jobs->head; job;) {
    Job* job_next = job->next;
    fp(jobs, job);
    job = job_next;
  }
}

/** 
 * Iterate over joblist, calling fp once on each job. 
 * Return the boolean AND of all of the results.
 * job_all does NOT short-circuit.
 * fp is a pointer to a function that takes two arguments:
 * a Joblist* and a Job*
*/
int job_all(JobList* jobs, int (*fp)(JobList*, Job*)) {
  assert(jobs);
  assert(fp);
  int all = 1;
  for (Job* job = jobs->head; job;) {
    Job* job_next = job->next;
    all = fp(jobs, job) && all;
    job = job_next;
  }
  return all;
}

