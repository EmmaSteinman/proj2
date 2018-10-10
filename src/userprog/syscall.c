#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void *get_vaddr(void *uaddr)
{
  if (uaddr == NULL || !is_user_vaddr(uaddr)) {
    return NULL;
  }

  struct thread* t = thread_current();
  uint32_t *pd = t->pagedir;
  void *vaddr = pagedir_get_page(pd, uaddr);

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
  printf ("system call!\n");
  putbuf("write()\n", 8);

  void *esp = f->esp;
  void *vaddr_esp = get_vaddr(esp);
  int syscall_number = *((int *) vaddr_esp);

  if (syscall_number == SYS_EXIT) {
    int status = *((int *) vaddr_esp + 4);
    exit(status);
  } else if (syscall_number == SYS_WRITE) {
    write(1, "write()\n", 8);
  }

  //thread_exit ();
}

void exit(int status)
{
  printf("hello: exit(%d)\n", status);
  thread_exit();
}

int write(int fd, const void *buffer, unsigned size)
{
  if (fd == 1) {
    putbuf(buffer, size);
  }

  return 0;
}