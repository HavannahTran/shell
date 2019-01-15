/**
 * CS 240 Shell Spells
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "command.h"
#include "joblist.h"
#include "terminal.h" 

#define NAME "bish"
#define PROMPT " bish-> "
#define HIST_FILE ".shell_history"

int shell_run_job(JobList* jobs, char** command, int foreground);
int shell_wait_fg(pid_t pid);
void job_do_foreground(JobList* jobs, Job* job);

/**
 * If command is built in, do it directly.  (Parts 1, 2, 3)
 *
 * Arguments:
 * jobs    - job list (Part 2-3)
 *           During Part 1, pass NULL for jobs when calling.
 * command - command to run
 *
 * Return:
 * 1 if the command is built in
 * 0 if the command is not built in
 */
int shell_builtin(JobList* jobs, char** command) {
  int builtin = 1;

  //help
  if (!strcmp(command[0],"help")){ 
    printf("cd\nexit\nhelp\nfg\nbg\n"); //prints five built-in functions
    printf("hi\ncatchphrase\njoke\n"); //prints out our fun built-ins

  }

  //exit
  else if (!strcmp(command[0],"exit")){
    if (joblist_empty(jobs)) {
      exit(0);
    }
    printf("There are unfinished jobs.\n");
  }

  //cd
  else if (!strcmp(command[0],"cd")){
    if (command[1] != NULL){
      //if there's an argument after "cd", go to that directory
      chdir(command[1]); 
    }
    else{
      char* home = getenv("HOME");
      chdir(home);
    }
  }

  //fg
  else if (!strcmp(command[0],"fg")){
    if (command[1] != NULL){ //1
      // if there is a jid provided 
      Jid jid = atoi(command[1]);
      Job* j = job_get(jobs, jid);
      if (j != NULL){ // 2: if the jid is a vaild job
	job_do_foreground(jobs, j);
	} // if 2

      else{ //if the job isn't vaild, we should print an error
	perror("Invalid Job");
      }
    } //if 1

    else{ // if no jid given
      //check if there are any jobs in the jobslist, because if there are none, then there won't be a "current job"
      if (!joblist_empty(jobs)){ 
	Job* current = job_get_current(jobs);
	job_do_foreground(jobs, current);
      }
    }
  }

    //bg
    else if (!strcmp(command[0],"bg")) {
      if (command[1] != NULL) { //if there is a jid given
	Jid jid = atoi(command[1]);
	Job* j = job_get(jobs, jid);
	if (j != NULL){ // if this jid maps to a valid job
	  if ((j -> status) == JOB_STATUS_STOPPED){ 
	    //if the job is stopped, make it a background job
	    kill(-(j->pid), SIGCONT); 
	  }
	  job_set_status(jobs, j, JOB_STATUS_BACKGROUND);
	  job_print(jobs,j);
	}
	else{ // if the j id does not map to a valid job, print an error
	  perror("Invalid Job");
	}
      }
      else {// if no jid is given
	if (!joblist_empty(jobs)) {
	  //check if the joblist is empty, because if so there will be no current job
	  Job* current = job_get_current(jobs);
	  if ((current -> status) == JOB_STATUS_STOPPED){ 
	    // if the current job is stopped, continue it
	    kill(-(current->pid), SIGCONT); 
	  }
	  //and set it as a background job
	  job_set_status(jobs, current, JOB_STATUS_BACKGROUND);
	  job_print(jobs,current);
	}
      }
    }

  //fun command 1: prints out "Hey Ashley" when you type the command hi
  else if (!strcmp(command[0],"hi")){
    printf("%s", ".......................................................................\n");
    printf("%s", ".##..##.######.##..##.........####...####..##..##.##.....######.##..##.\n");
    printf("%s", ".##..##.##......####.........##..##.##.....##..##.##.....##......####.. \n");
    printf("%s", ".######.####.....##..........######..####..######.##.....####.....##...\n");
    printf("%s", ".##..##.##.......##..........##..##.....##.##..##.##.....##.......##...\n");
    printf("%s", ".##..##.######...##..........##..##..####..##..##.######.######...##...\n");
    printf("%s", ".......................................................................\n");
  }

  //fun command 2: prints out "Right on" when you type the command catchphrase
  else if (!strcmp(command[0],"catchphrase")){
      printf("%s", "Right on\n");
  }

  //fun command 3: prints out a cute lacrosse joke if the command "joke" is entered
  else if (!strcmp(command[0],"joke")){
    printf("%s","Why is a lacrosse field the coolest place to be?\n");
    sleep(3);
    printf("%s","Because its full of fans!\n");
  }


 else{ //user doesn't type in any of the 3 built-in commands
    builtin = 0;
  }

  if (builtin) {
    //save the current job and then delete after it terminates immediately
    pid_t pid = getpid();
    Job* current = job_save(jobs, pid, command, JOB_STATUS_FOREGROUND);
    job_delete(jobs, current);
  }
  return builtin;
}

void job_do_foreground(JobList* jobs, Job* job) {
  if ((job -> status) == JOB_STATUS_STOPPED){ //check if the job has been stopped
     kill(-(job->pid), SIGCONT); //continue the job
     job_set_status(jobs,job,JOB_STATUS_FOREGROUND);
     job_print(jobs, job);
  }
  // if the jid maps to a background job OR is a stopped job, then we want to bring that job to the foreground
  job_set_status(jobs,job,JOB_STATUS_FOREGROUND);
  term_give(jobs, job);
  int stopped = shell_wait_fg(job->pid);
  term_take(jobs, job);

  if (stopped) { // if we CTRL-Z the process that has newly been brought to the foreground 
     job_set_status(jobs, job, JOB_STATUS_STOPPED); // we want to set the status
     job_print(jobs, job); // and also print out the stopped job
  }

  else { // if the background job is now a foreground job and is not stopped
     job_delete(jobs, job); //delete from the jobslist
  }
}

/**
 * Place the process with the given pid in the foreground and wait for
 * it to terminate (Part 1) or stop (Part 3). Do not use this
 * functions for other (non-blocking) types of waiting.
 *
 * Exit in error if an error occurs while waiting (Part 1).
 *
 * Arguments:
 * pid - wait for the process with this process ID.
 *
 * Return:
 * 0 if process pid has terminated  (Part 1)
 * 1 if process pid has stopped     (Part 3)
 */
int shell_wait_fg(pid_t pid) {
  int status;
  if (-1 == waitpid(pid, &status, WUNTRACED)) {
      perror("waitpid");
      exit(1);
    }
  else {
    if (WIFEXITED(status)) {
      return 0;
    }
    else {
      return 1;
    }
  }
}


/**
 * Fork and exec the requested program in the foreground (Part 1)
 * or background (Part 2).
 *
 * Use shell_wait_fg to do all *foreground* waiting.
 *
 * Exit in error if an error occurs while forking (Part 1).
 *
 * Arguments:
 * jobs       - job list for saving or deleting jobs (Part 2-3)
 *              During Part 1, pass NULL for jobs when calling.
 * command    - command array of executable and arguments
 * foreground - indicates foregounrd (1) vs. background (0)
 *
 * Return:
 * 0 if the foreground process terminated    (Part 1)
 * 0 if a background process was forked      (Part 2)
 * 1 if the foreground process stopped       (Part 3)
 */
int shell_run_job(JobList* jobs, char** command, int foreground) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(-1);
  }
  if (pid != 0) { //if it's the parent process
    if (foreground){
      //save foreground job, wait for it to finish, and then delete after
      //it terminates immediately
      Job* current = job_save(jobs,pid,command,JOB_STATUS_FOREGROUND);
      term_give(jobs, current);
      int stopped = shell_wait_fg(pid);
      term_take(jobs, current);

      if (stopped) {
	job_set_status(jobs, current, JOB_STATUS_STOPPED);
	job_print(jobs, current);
      }
      else {
	job_delete(jobs, current);
      }
    }
    else {
      //save background job and print job
      Job* current = job_save(jobs,pid,command,JOB_STATUS_BACKGROUND);
      job_print(jobs, current);
    }
  } 
  else { //child process runs this
    term_child_init(jobs, foreground);
    execvp(command[0], command); //ok issue might be here too
    //would only print error if execvp can't execute and it returns to this
    //else statement
    perror("execvp");
    exit(1);
  }
 
  return 0; // part 1 + 2, returns 0
}

/**
 * Helper function to determine whether a process is finished
 * and reap and delete if necessary
 * Passed to job_iter() as an argument
 *
 * Arguments:
 * jobs - the joblist
 * job - the process whose status is being checked
 *
 * Return:
 * void
 */
void job_setDelete (JobList* jobs, Job* job){
    int status;
    pid_t pid = (job->pid);
    int result = waitpid(pid, &status, WNOHANG | WUNTRACED);
    if (result < 0) {
      perror("waitpid");
      exit(1); 
    }
    else if (result) {
      //sets job status to done and deletes job
      job_set_status(jobs, job, JOB_STATUS_DONE);
      job_print(jobs, job);
      job_delete(jobs,job);
    }
}


/**
 * Main shell loop: read, parse, execute.
 *
 * Arguments:
 * argc - number of command-line arguments for the shell.
 * argv - array of command-line arguments for the shell.
 *
 * Return:
 * exit status - 0 for normal, non-zero for error.
 */
int main(int argc, char** argv) {
  // Load history if available.
  using_history();
  read_history(HIST_FILE);

  //create instance of JobList
  JobList* jobs = joblist_create();
  term_shell_init(jobs);

  // Until ^D (EOF), read command line.
  char* line = NULL;
  char cwd[1024];
  if (getcwd(cwd,sizeof(cwd)) == NULL){
      perror("getcwd() error");
  }
    while ((line = readline(strcat(cwd,PROMPT))) || !joblist_empty(jobs)) {
    if (line) { //if line isn't EOF
      // Add line to history.
      add_history(line);
      int fg = -1;
      char** command = command_parse(line, &fg);
      // Free command line.
      free(line);

      if (command[0] != NULL){
	int check = shell_builtin(jobs,command);
	if (!check) { //if not builtin command, call shell_run_job
	  shell_run_job(jobs, command, fg);
	  //iterate through jobs in joblist at the end of every command
	  //to set appropriate jobs to JOB_STATUS_DONE and to delete them
	  job_iter(jobs,job_setDelete); //moved within this statement
	}
      }
    }
    else {
      printf("There are unfinished jobs.\n");
    }
    if (getcwd(cwd,sizeof(cwd)) == NULL){
      perror("getcwd() error");
    }
  }

  //EOF
  exit(0);

  return 0;
}

