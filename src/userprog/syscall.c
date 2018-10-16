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

/*  REMAINING SYSTEM CALLS TO IMPLETMENT

pid_t exec (const char *cmd_line);
bool create(const char *file, unsigned initial_size);
bool remove(const cahr *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigner size);
unsigned tell(int fd);
void close(int fd);
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
      // printf("%p %p %p %p\n", esp, esp + 4, esp + 8, esp + 12);
      // printf("%p %p %p %p\n", get_vaddr(esp), get_vaddr(esp+4), get_vaddr(esp+8),
      //        get_vaddr(esp+12));
      // printf("%10d %10d %p %10d\n", syscall_number, fd, buffer, size);
      // printf("buffer = %p - %p\n", esp + 8, buffer);

      write(fd, *buffer, size);

      break;
    }
    case SYS_HALT:
    {
      halt();
    }
    case SYS_OPEN:
    {
      char **file = (char *) get_vaddr(esp + 4);

      open(*file);
      break;
    }
    /*
    case SYS_CLOSE:
    {
      int fd = *((int *) get_vaddr(esp+4));

      close(fd);
      break;
    }
    case SYS_EXEC:
    {
      char *cmd_line = *((char *) get_vaddr(esp + 4));

      exec(cmd_line);
      break;
    }
    case SYS_WAIT:
    {
      pid_t pid = *(get_vaddr(esp + 4));

      wait(pid);
      break;
    }
    case SYS_CREATE:
    {
      char *file = *((char *) get_vaddr(esp + 4));
      int initial_size = *((int *) get_vaddr(esp + 8));

      create(file, initial_size);
      break;
    }
    case SYS_REMOVE:
    {
      char *file = *((char *) get_vaddr(esp + 4));

      remove(file);
      break;
    }
    case SYS_FILESIZE:
    {
      int fd = *((int *) get_vaddr(esp + 4));

      filesize(fd);
      break;
    }
    case SYS_READ:
    {
      int fd = *((int *) get_vaddr(esp + 4));
      void **buffer = get_vaddr(esp + 8);
      int size = *((int *) get_vaddr(esp + 12));

      read(fd, buffer, size);
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

      tell(fd);
      break;
    }
    case SYS_CLOSE:
    {
      int fd = *((int *) get_vaddr(esp + 4));

      clsoe(fd);
      break;
    }*/


  }

  //thread_exit ()
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
  struct thread *t = thread_current();
  int fd = t->current_fd;
  struct file *file_1 = filesys_open(file);
  t->fd_array[fd] = file_1;
  t->current_fd ++;

  return fd;
}

/*
int filesize(int fd)
{
  //Convert fd to file
  //off_t size = file_length(file)
  //return size;
  return 1;
}
*/
