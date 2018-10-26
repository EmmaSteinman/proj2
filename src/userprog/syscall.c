#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "process.h"
#include "devices/input.h"

#define DEBUG 0

static void syscall_handler (struct intr_frame *);
static void *get_vaddr(void *uaddr);
static void *sc_get_arg(int pos, void *esp);
static void *sc_get_char_arg(int pos, void *esp);

void halt(void);
void exit(int status);
int write(int fd, const void *buffer, unsigned size);
int open(const char *file);
bool create(const char *file, unsigned initial_size);
void close(int fd);
int filesize(int fd);
unsigned tell(int fd);
void seek(int fd, unsigned position);
pid_t exec (const char *cmd_line);
bool remove(const char *file);
int read(int fd, void *buffer, unsigned size);
int wait(pid_t pid);

//==========================================================
// get_vaddr
// getting virtual address from user memory
//==========================================================
static void *get_vaddr(void *uaddr)
{
  if (uaddr == NULL || !is_user_vaddr(uaddr)) {
    return NULL;
  }

  struct thread* t = thread_current();
  void *vaddr = pagedir_get_page(t->pagedir, uaddr);

  return vaddr;
}

//==========================================================
// sc_get_arg
// gets ith argument from the stack, checking validity 
//  before returning
//==========================================================
static void *sc_get_arg(int pos, void *esp)
{
  void *arg = esp + sizeof(void *) * pos;
  void *vaddr_arg = get_vaddr(arg);

  if (vaddr_arg == NULL || get_vaddr(arg + 3) == NULL) {            //check if entire 4 bytes are valid
    exit(-1);
  }

  return vaddr_arg;
}

//==========================================================
// sc_get_char_arg
// gets ith character argument from stack, checking 
//  validity of address and contents before returning
//==========================================================
static void *sc_get_char_arg(int pos, void *esp)
{
  void **str_addr = sc_get_arg(pos, esp);
  void *str = get_vaddr(*str_addr);
  char *test_str = get_vaddr(*str_addr);

  if (str == NULL || get_vaddr(*str_addr + strlen(test_str)) == NULL) {         //checking if contents are valid
    exit(-1);
  }

  return  str;
}

//==========================================================
// syscall_init
// initializes syscall handler 
//==========================================================
void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

//==========================================================
// syscall_handler
// handles system calls, calling appropriate functions 
//  depending on what values are on the stack
//==========================================================
static void
syscall_handler (struct intr_frame *f)
{
  void *esp = f->esp;
  void *vaddr_esp = get_vaddr(esp);
  if (vaddr_esp == NULL || get_vaddr(esp + 3) == NULL) {      //checking validity of all 4 bytes
    exit(-1);
  }

  int syscall_number = *((int *) vaddr_esp);

  int retval = 0;
  bool retval_bool = true;
  bool has_retval = false;
  bool has_retval_bool = false;


  switch (syscall_number) {                           //gets arguments from stack and calls function
    case SYS_EXIT:
    {
      int status = *((int *) sc_get_arg(1, esp));
      exit(status);
      break;
    }
    case SYS_WRITE:
    {
      int fd = *((int *) sc_get_arg(1, esp));
      void *buffer = sc_get_char_arg(2, esp);
      int size = *((int *) sc_get_arg(3, esp));

      retval = write(fd, buffer, size);
      has_retval = true;

      break;
    }
    case SYS_HALT:
    {
      halt();
    }
    case SYS_OPEN:
    {
      char *filename = sc_get_char_arg(1, esp);

      retval = open(filename);
      has_retval = true;
      break;
    }
    case SYS_CREATE:
    {
      char *file = sc_get_char_arg(1, esp);
      int initial_size = *((int *) sc_get_arg(2, esp));

      retval_bool = create(file, initial_size);
      has_retval_bool = true;
      break;
    }

    case SYS_EXEC:
    {
      char *cmd_line = sc_get_char_arg(1, esp);

      retval = exec(cmd_line);
      has_retval = true;
      break;
    }
    case SYS_WAIT:
    {
      pid_t pid = *(pid_t*) (sc_get_arg(1, esp));

      retval = wait(pid);
      has_retval = true;
      break;
    }
    case SYS_REMOVE:
    {
      char *filename = sc_get_char_arg(1, esp);

      retval_bool = remove(filename);
      has_retval_bool = true;
      break;
    }
    case SYS_READ:
    {
      int fd = *((int *) sc_get_arg(1, esp));
      void *buffer = sc_get_char_arg(2, esp);
      int size = *((int *) sc_get_arg(3, esp));

      retval = read(fd, buffer, size);
      has_retval = true;
      break;
    }
    case SYS_FILESIZE:
    {
      int fd = *((int *) sc_get_arg(1, esp));

      retval = filesize(fd);
      has_retval = true;
      break;
    }
    case SYS_SEEK:
    {
      int fd = *((int *) sc_get_arg(1, esp));
      int position = *((int *) sc_get_arg(2, esp));

      seek(fd, position);
      break;
    }
    case SYS_TELL:
    {
      int fd = *((int *) sc_get_arg(1, esp));

      retval = tell(fd);
      has_retval = true;

      break;
    }
    case SYS_CLOSE:
    {
      int fd = *((int *) sc_get_arg(1, esp));

      close(fd);
    }
  }

  // put the return value back on the user's stack if needed
  if (has_retval) {
    f->eax = retval;
  }
  else if (has_retval_bool) {
    f->eax = retval_bool;
  }
}

//==========================================================
// halt
// terminates Pintos
//==========================================================
void halt(void)
{
  shutdown_power_off();
}

//==========================================================
// exit
// terminates current user program
//==========================================================
void exit(int status)
{
  thread_current()->exit_status = status;
  thread_exit();
}

//==========================================================
// exec
// create new process running executable file
//==========================================================
pid_t exec (const char *cmd_line)
{
  if (cmd_line == NULL)
    exit(-1);

  struct thread* current = thread_current();
  struct process* parent = get_process(current->tid);
  struct child* new_child = palloc_get_page(0);

  if (new_child == NULL) {            //no more memory to allocate
    return -1;
  }

  new_child->parent_pid = current->tid;       //set up new_child struct
  new_child->exit_status = -1;
  sema_init(&new_child->child_sema, 0);
  sema_init(&new_child->load_done_sema, 0);
  process_add_child(new_child);

  pid_t p = process_execute(cmd_line);
  if (p == TID_ERROR) {                                     //if process failed
    process_remove_child(&parent->child_lock, new_child);
    palloc_free_page(new_child);
    return -1;
  }

  new_child->pid = p;
  sema_down(&new_child->load_done_sema);

  if (new_child->load_success) {
    // load sucessful
    return p;
  } else {
    // load failed - free resources
    process_remove_child(&parent->child_lock, new_child);
    palloc_free_page(new_child);  
    return -1;
  }
}

//==========================================================
// wait
// waits for child process and gets its exit status
//==========================================================
int wait(pid_t pid)
{
  if (pid == -1){
    return -1;
  }

  struct thread *t = thread_current();
  struct process *parent = get_process(t->tid);
  int exit_status;

  struct list_elem *child_elem = get_child_process(parent->pid, pid);
  if (child_elem == NULL) {                     //not a valid child of parent
    return -1;
  }
  struct child *child_proc = list_entry(child_elem, struct child, childelem);

  sema_down(&child_proc->child_sema);       //waits for child to exit

  exit_status = child_proc->exit_status;

  process_remove_child(&parent->child_lock, child_proc);        //free resources
  palloc_free_page(child_proc);

  return exit_status;
}

//==========================================================
// write
// writes to open file
//==========================================================
int write(int fd, const void *buffer, unsigned size)
{
  struct thread *t = thread_current();
  struct file *file;

  if (buffer == NULL) {
    exit(-1);
  }
  if (fd == 1) {                //stdout
    putbuf(buffer, size);
    return size;
  }
  if(fd < t->current_fd){
     file = t->fd_array[fd];
  }
  else{
    file = NULL;
  }
  if (file == NULL){            //invalid file descriptor
    exit(-1);
  }

  return file_write(file, buffer, size);
}

//==========================================================
// open
// opens file
//==========================================================
int open(const char *file)
{
  if (file == NULL) {
    exit(-1);
  }

  struct thread *t = thread_current();
  int fd = t->current_fd;                     //get next available file descriptor

  struct file *file_1 = filesys_open(file);
  if (file_1 == NULL) {                         //open failed
    return -1;
  }

  t->fd_array[fd] = file_1;
  t->current_fd++;                          //update file descriptor array 

  return fd;
}

//==========================================================
// close
// closes file
//==========================================================
void close(int fd){
  struct thread *t = thread_current();
  struct file *file;
  if(fd < t->current_fd){
     file = t->fd_array[fd];              //update file descriptor array
     t->fd_array[fd] = NULL;
  }
  else{
    file = NULL;
  }
  file_close(file);
}

//==========================================================
// filesize
// returns size of file
//==========================================================
int filesize(int fd)
{
  struct thread *t = thread_current();
  struct file *file;

  if(fd < t->current_fd){               //finds file in array
     file = t->fd_array[fd];
  }
  else{
    file = NULL;
  }
  off_t size = file_length(file);
  return size;
}

//==========================================================
// tell
// returns next byte to be read or written to
//==========================================================
unsigned tell(int fd){
  struct thread *t = thread_current();
  struct file *file;

  if(fd < t->current_fd){           //gets file
     file = t->fd_array[fd];
  }
  else{
    file = NULL;
  }
  off_t tell = file_tell(file);

  return tell;
}

//==========================================================
// seek
// changes next byte to be written to or read to position
//  from beginning of file
//==========================================================
void seek(int fd, unsigned position){
  struct thread *t = thread_current();
  struct file *file;

  if(fd < t->current_fd){
     file = t->fd_array[fd];
  }
  else{
    file = NULL;
  }

  file_seek(file, position);
}

//==========================================================
// create
// creates a new file
//==========================================================
bool create(const char *file, unsigned initial_size){
  if (file == NULL) {
    exit(-1);
  }

  return filesys_create(file, initial_size);
}

//==========================================================
// read
// reads from an open file
//==========================================================
int read(int fd, void *buffer, unsigned size){
  struct thread *t = thread_current();
  struct file *file;
  char *letter = buffer;

  if (buffer == NULL) {
    exit(-1);
  }

  if (fd == 0){                         //stdin
    for(int i  =0;i < (int)size; i++){
      letter[i] = input_getc();
    }
    return size;
  }

  if(fd < t->current_fd){
     file = t->fd_array[fd];  
  }
  else{
    file = NULL;
  }
  if (file == NULL){
    exit(-1);
  }

  return file_read(file, buffer, size);
}

//==========================================================
// remove
// deletes file
//==========================================================
bool remove(const char *file){
  if (file == NULL) {
    exit(-1);
  }

  return filesys_remove(file);
}
