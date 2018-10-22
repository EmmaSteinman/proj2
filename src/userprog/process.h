#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
typedef int pid_t;

struct process {
  pid_t pid;
  pid_t parent_pid;
  struct list child_list;
  struct list_elem procelem;
};

struct child {
  pid_t pid;
  pid_t parent_pid;
  int exit_status;
  struct semaphore child_sema;
  struct list_elem childelem;
};

struct list process_list;

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct process* get_process(pid_t p);
struct list_elem* get_child_process(pid_t parent_pid, pid_t child_pid);
void process_add(struct process* proc);
void process_add_child(struct child* ch);
#endif /* userprog/process.h */
