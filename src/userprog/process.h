#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
typedef int pid_t;

struct process {
  pid_t pid;                      //process identifier
  pid_t parent_pid;               //pid of parent
  struct list child_list;         //all of process' children
  struct list_elem procelem;      
  struct lock child_lock;         //allows for synchronization
};

struct child {
  pid_t pid;                      //process identifier
  pid_t parent_pid;               //pid of parent
  int exit_status;                //exit status (for use of parent)
  struct semaphore child_sema;    //allows for synchronization
  struct list_elem childelem;
  struct semaphore load_done_sema;  //synchronization due to loading
  bool load_success;
};

struct list process_list;             //holding all current processes
struct lock process_list_lock;        //allows for synchronization
struct lock filesys_lock;

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct process* get_process(pid_t p);
struct list_elem* get_child_process(pid_t parent_pid, pid_t child_pid);
void process_add(struct process* proc);
void process_remove(struct process* proc);
void process_add_child(struct child* ch);
void process_remove_child(struct lock *child_lock, struct child *child);
#endif /* userprog/process.h */
