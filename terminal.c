/**
 * CS 240 Shell Spells
 * Ben Wood, 2015
 *
 * Terminal and signals management
 *
 * See terminal.h for documentation and usage.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>

#include "joblist.h"
#include "terminal.h"

pid_t term_get(int term) {
  pid_t pg = tcgetpgrp(term);
  if (pg < 0) {
    perror("tcgetpgrp");
    exit(1);
  }
  return pg;
}

void term_set(int term, pid_t pg) {
  if (tcsetpgrp(term, pg) < 0) {
    perror("tcsetpgrp");
    exit(1);
  }
}

void term_shell_init(JobList* jobs) {
  assert(jobs);
  jobs->term = STDIN_FILENO;
  jobs->interactive = isatty(jobs->term);

  if (jobs->interactive) {
    pid_t t = term_get(jobs->term);
    pid_t g = getpgrp();
    if (g < 0) {
      perror("getpgrp < 0");
      exit(1);
    }
    while (t != g) {
      if (kill(-g, SIGTTIN) < 0) {
        perror("kill");
      }
      t = term_get(jobs->term);
      g = getpgrp();
      if (g < 0) {
        perror("getpgrp < 0");
        exit(1);
      }
    }
    
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    
    jobs->pgid = getpid();
    if (setpgid(jobs->pgid, jobs->pgid) < 0) {
      perror("shell setpgid(self,self)");
      exit(1);
    }
    
    term_set(jobs->term, jobs->pgid);
    tcgetattr(jobs->term, &jobs->tmodes);
  }
  
}


void term_give(JobList* jobs, Job* job) {
  assert(jobs);
  if (jobs->interactive) {
    pid_t child = job->pid;
    pid_t child_pgid = getpgid(child);
    if (child_pgid == jobs->pgid || child_pgid == 0) {
      if (setpgid(child, child) < 0) {
        perror("child setpgid(child,child)");
        exit(1);
      }
    }
    term_set(jobs->term, child);
    if (job->status == JOB_STATUS_STOPPED) {
      tcsetattr(jobs->term, TCSADRAIN, &job->tmodes);
    }
  }
}

void term_take(JobList* jobs, Job* job) {
  assert(jobs);
  if (jobs->interactive) {
    term_set(jobs->term, jobs->pgid);
    if (job->status != JOB_STATUS_DONE) {
      tcgetattr(jobs->term, &job->tmodes);
    }
    tcsetattr(jobs->term, TCSADRAIN, &jobs->tmodes);
  }
}

void term_child_init(JobList* jobs, int foreground) {
  assert(jobs);
  if (jobs->interactive) {
    pid_t pid = getpid();
    if (getpgrp() != pid) {
      if (setpgid(pid, pid) < 0) {
        perror("child setpgid(child,child)");
        exit(1);
      }
    }
    if (foreground) {
      term_set(jobs->term, pid);
    }

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
  }
}
