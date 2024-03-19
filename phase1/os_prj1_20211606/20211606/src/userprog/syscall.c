#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  switch(*(uint32_t*)(f->esp))
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      exit(*(uint32_t*)(f->esp + 4));
      break;
    case SYS_EXEC:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = exec((const char *)(*(uint32_t *)(f->esp + 4)));
      break;
    case SYS_WAIT:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = wait((tid_t)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_READ:
      check_valid_address(*(uint32_t*)(f->esp + 20));
      check_valid_address(*(uint32_t*)(f->esp + 24));
      check_valid_address(*(uint32_t*)(f->esp + 28));
      f->eax = read((int)*(uint32_t *)(f->esp + 20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      break;
    case SYS_WRITE:
      check_valid_address(*(uint32_t*)(f->esp + 20));
      check_valid_address(*(uint32_t*)(f->esp + 24));
      check_valid_address(*(uint32_t*)(f->esp + 28));
      f->eax = write((int)*(uint32_t *)(f->esp + 20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      break;
    case SYS_FIBO:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = fibonacci((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_MAXNUM:
      check_valid_address(*(uint32_t*)(f->esp + 28));
      check_valid_address(*(uint32_t*)(f->esp + 32));
      check_valid_address(*(uint32_t*)(f->esp + 36));
      check_valid_address(*(uint32_t*)(f->esp + 40));
      f->eax = max_of_four_int((int)*(uint32_t *)(f->esp + 28), (int)*(uint32_t *)(f->esp + 32), (int)*(uint32_t *)(f->esp + 36), (int)*(uint32_t *)(f->esp + 40));
      break;
    default:
      break;
  
  }
}

// The function which checks the validity of given address
// userprog/pagedir.c and threads/vaddr.h
void check_valid_address(const void *vaddr)
{
  //if(vaddr == NULL) exit(-1);
  if(is_kernel_vaddr(vaddr)) exit(-1);
  //(pagedir_get_page(thread_current()->pagedir, vaddr) == NULL) exit(-1);
}

void halt()
{
  shutdown_power_off();
}

void exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit();
}

tid_t exec(const char* file)
{
  return process_execute(file);
}

int wait (tid_t child_tid)
{
  return process_wait(child_tid);
}

int read (int fd, void *buffer, unsigned length)
{
  unsigned Count = 0;
  if(!fd)
  {
    while(Count < length)
    {
      if((((char *)buffer)[Count] = input_getc()) == '\0')
        break;
      Count++;
    }
  }
  return Count;
}
int write (int fd, const void *buffer, unsigned length)
{
  if(fd == 1)
  {
    putbuf(buffer, length);
    return length;
  }
  else return -1;
}
int fibonacci(int a)
{
    int pre = 1;
    int tmp;
    int cur = 2;
    if(a == 1 || a == 2) return 1;
    for(int i = 3; i < a; i++)
    {
        tmp = cur;
        cur += pre;
        pre = tmp;
    }
    return cur;
}
int max_of_four_int(int a, int b, int c, int d)
{
    int Max = a;
    if(Max < b) Max = b;
    if(Max < c) Max = c;
    if(Max < d) Max = d;
    return Max;
}