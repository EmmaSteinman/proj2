#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "../filesys/file.h"
#include "../filesys/filesys.h"

#define DEBUG 0

static void syscall_handler (struct intr_frame *);
void *get_vaddr(void *uaddr);
void halt(void);
void exit(int status);
int write(int fd, const void *buffer, unsigned size);
int open(const char *file);
bool create(const char *file, unsigned initial_size);
void close(int fd);
int filesize(int fd);
unsigned tell(int fd);
void seek(int fd, unsigned position);

/*  REMAINING SYSTEM CALLS TO IMPLETMENT

pid_t exec (const char *cmd_line);
bool remove(const cahr *file);
int read(int fd, void *buffer, unsigner size);
int wait(pid_t pid);
*/

void *get_vaddr(void *uaddr)
{
  if (uaddr == NULL || !is_user_vaddr(uaddr)) {
    return NULL;
  }

  struct thread* t = thread_current();
  void *vaddr = pagedir_get_page(t->pagedir, uaddr);

  return vaddr;
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  #if DEBUG
  printf ("system call!\n");
  #endif

  void *esp = f->esp;
  void *vaddr_esp = get_vaddr(esp);
  int syscall_number = *((int *) vaddr_esp);

  int retval = 0;
  bool retval_bool = true;  //DOES IT MATTER WHAT IT IS INITIALIZED TO
  bool has_retval = false;
  bool has_retval_bool = false;


  switch (syscall_number) {
    case SYS_EXIT:
    {
      int status = *((int *) get_vaddr(esp + 4));
      exit(status);
      break;
    }
    case SYS_WRITE:
    {
      int fd = *((int *) get_vaddr(esp + 4));
      void **buffer = get_vaddr(esp + 8);
      int size = *((int *) get_vaddr(esp + 12));

      retval = write(fd, *buffer, size);
      has_retval = true;

      break;
    }
    case SYS_HALT:
    {
      halt();
    }
    case SYS_OPEN:
    {
      char **filename_addr_ptr = get_vaddr(esp + 4);
      char *filename = get_vaddr(*filename_addr_ptr);

      retval = open(filename);
      has_retval = true;
      break;
    }
    case SYS_CREATE:
    {
      char *file = *((char *) get_vaddr(esp + 4));
      int initial_size = *((int *) get_vaddr(esp + 8));

      retval_bool = create(file, initial_size);
      has_retval_bool = true;
      break;
    }
    /*
    case SYS_EXEC:
    {
      char *cmd_line = *((char *) get_vaddr(esp + 4));

      exec(cmd_line);
      break;
    }
    case SYS_WAIT:
    {
      pid_t pid = *(get_vaddr(esp + 4));

      retval = wait(pid);
      has_retval = true;
      break;
    }
    case SYS_REMOVE:
    {
      char *file = *((char *) get_vaddr(esp + 4));

      retval_bool = remove(file);
      has_retval_bool = true;
      break;
    }
    case SYS_READ:
    {
      int fd = *((int *) get_vaddr(esp + 4));
      void **buffer = get_vaddr(esp + 8);
      int size = *((int *) get_vaddr(esp + 12));

      retval = read(fd, buffer, size);
      has_retval = true;

      break;
    }
    */
    case SYS_FILESIZE:
    {
      int fd = *((int *) get_vaddr(esp + 4));

      retval = filesize(fd);
      has_retval = true;
      break;
    }
    case SYS_SEEK:
    {
      int fd = *((int *) get_vaddr(esp + 4));
      int position = *((int *) get_vaddr(esp + 8));

      seek(fd, position);
      break;
    }
    case SYS_TELL:
    {
      int fd = *((int *) get_vaddr(esp + 4));

      retval = tell(fd);
      has_retval = true;

      break;
    }
    case SYS_CLOSE:
    {
      int fd = *((int *) get_vaddr(esp + 4));

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

int write(int fd, const void *buffer, unsigned size)
{
  #if DEBUG
  printf("write()\n");
  #endif

  if (fd == 1) {
    //printf("buffer: %p\n", buffer);
    //hex_dump(buffer-100, buffer-100, 1000, 1);
    putbuf(buffer, size);
    return size;
  }
  /*
  else{
    int current = tell(fd);
    int file_size = filesize(fd);

    if(file_size-current>size){
      return size;
      }
    else{
      return max(file_size-current|0);
    }
  }*/
  return 0;
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
  struct file *file = thread_current()->fd_array[fd];
  off_t size = file_length(file);
  return size;
}

unsigned tell(int fd){
  struct file *file = thread_current()->fd_array[fd];
  off_t tell = file_tell(file);

  return tell;
}

void seek(int fd, unsigned position){
  struct file *file = thread_current()->fd_array[fd];

  file_seek(file, position);
}

bool create(const char *file, unsigned initial_size){
  if (file == NULL) {
    exit(-1);
  }

  return filesys_create(file, initial_size);
}
