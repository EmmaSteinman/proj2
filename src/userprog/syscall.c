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


static void *get_vaddr(void *uaddr)
{
  if (uaddr == NULL || !is_user_vaddr(uaddr)) {
    return NULL;
  }

  struct thread* t = thread_current();
  void *vaddr = pagedir_get_page(t->pagedir, uaddr);

  return vaddr;
}

static void *sc_get_arg(int pos, void *esp)
{
  void *arg = esp + sizeof(void *) * pos;
  void *vaddr_arg = get_vaddr(arg);

  if (vaddr_arg == NULL || get_vaddr(arg + 3) == NULL) {
    exit(-1);
  }

  return vaddr_arg;
}

static void *sc_get_char_arg(int pos, void *esp)
{
  void **str_addr = sc_get_arg(pos, esp);
  void *str = get_vaddr(*str_addr);
  char *test_str = get_vaddr(*str_addr);

  if (str == NULL || get_vaddr(*str_addr + strlen(test_str)) == NULL) {
    exit(-1);
  }

  return  str;
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  void *esp = f->esp;
  void *vaddr_esp = get_vaddr(esp);
  if (vaddr_esp == NULL || get_vaddr(esp + 3) == NULL) {
    exit(-1);
  }

  int syscall_number = *((int *) vaddr_esp);

  int retval = 0;
  bool retval_bool = true;
  bool has_retval = false;
  bool has_retval_bool = false;


  switch (syscall_number) {
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

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  thread_current()->exit_status = status;
  thread_exit();
}

pid_t exec (const char *cmd_line)
{
  if (cmd_line == NULL)
    exit(-1);

  struct thread* current = thread_current();
  struct child* new_child = palloc_get_page(0);

  new_child->parent_pid = current->tid;
  new_child->exit_status = -1;
  sema_init(&new_child->child_sema, 0);
  process_add_child(new_child);

  pid_t p = process_execute(cmd_line);
  if (p == TID_ERROR) {
    palloc_free_page(new_child);
    return -1;
  }


  new_child->pid = p;
  struct process *new_process = get_process(p);
  sema_down(&new_process->load_done_sema);

  if (new_process->load_success) {
    return p;
  }

  return -1;
}


int wait(pid_t pid)
{
  if (pid == -1){
    return -1;
  }

  struct thread *t = thread_current();
  struct process *parent = get_process(t->tid);
  int exit_status;

  struct list_elem *child_elem = get_child_process(parent->pid, pid);
  if (child_elem == NULL) {
    return -1;
  }
  struct child *child_proc = list_entry(child_elem, struct child, childelem);

  sema_down(&child_proc->child_sema);

  exit_status = child_proc->exit_status;

  list_remove(child_elem);
  palloc_free_page(child_proc);

  return exit_status;
}

int write(int fd, const void *buffer, unsigned size)
{
  struct thread *t = thread_current();
  struct file *file;

  if (buffer == NULL) {
    exit(-1);
  }
  if (fd == 1) {
    putbuf(buffer, size);
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

  return file_write(file, buffer, size);
}

int open(const char *file)
{
  if (file == NULL) {
    exit(-1);
  }

  struct thread *t = thread_current();
  int fd = t->current_fd;

  struct file *file_1 = filesys_open(file);
  if (file_1 == NULL) {
    return -1;
  }

  t->fd_array[fd] = file_1;
  t->current_fd++;

  return fd;
}

void close(int fd){
  struct thread *t = thread_current();
  struct file *file;
  if(fd < t->current_fd){
     file = t->fd_array[fd];
     t->fd_array[fd] = NULL;
  }
  else{
    file = NULL;
  }
  file_close(file);
}

int filesize(int fd)
{
  struct thread *t = thread_current();
  struct file *file;

  if(fd < t->current_fd){
     file = t->fd_array[fd];
  }
  else{
    file = NULL;
  }
  off_t size = file_length(file);
  return size;
}

unsigned tell(int fd){
  struct thread *t = thread_current();
  struct file *file;

  if(fd < t->current_fd){
     file = t->fd_array[fd];
  }
  else{
    file = NULL;
  }
  off_t tell = file_tell(file);

  return tell;
}

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

bool create(const char *file, unsigned initial_size){
  if (file == NULL) {
    exit(-1);
  }

  return filesys_create(file, initial_size);
}

int read(int fd, void *buffer, unsigned size){
  struct thread *t = thread_current();
  struct file *file;
  char *letter = buffer;

  if (buffer == NULL) {
    exit(-1);
  }

  if (fd == 0){
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

bool remove(const char *file){
  if (file == NULL) {
    exit(-1);
  }

  return filesys_remove(file);
}
