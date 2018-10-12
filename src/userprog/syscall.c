#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#define DEBUG 0

static void syscall_handler (struct intr_frame *);
void *get_vaddr(void *uaddr);
void exit(int status);
int write(int fd, const void *buffer, unsigned size);
void halt(void);

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
  }

  return 0;
}

