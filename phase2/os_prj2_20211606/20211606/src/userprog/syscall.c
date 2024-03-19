#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/off_t.h"
/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };
struct lock file_lock;
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
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
    // Create a file
    case SYS_CREATE:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      check_valid_address(*(uint32_t*)(f->esp + 8));
      f->eax = create((const char *)(*(uint32_t *)(f->esp + 4)), (unsigned)*((uint32_t *)(f->esp + 8)));
      break;
    // Delete a file
    case SYS_REMOVE:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = remove((const char *)(*(uint32_t *)(f->esp + 4)));
      break;
    // Open a file
    case SYS_OPEN:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = open((const char *)(*(uint32_t *)(f->esp + 4)));
      break;
    // Close a file
    case SYS_CLOSE:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      close((int)*(uint32_t*)(f->esp + 4));
      break;
    // Obtain a file's size
    case SYS_FILESIZE:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = filesize((int)*(uint32_t*)(f->esp + 4));
      break;
    // Change position in a file
    case SYS_SEEK:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      check_valid_address(*(uint32_t*)(f->esp + 8));
      seek((int)*(uint32_t*)(f->esp + 4), (unsigned)*(uint32_t*)(f->esp + 8));
      break;
    // Report current position in a file
    case SYS_TELL:
      check_valid_address(*(uint32_t*)(f->esp + 4));
      f->eax = tell((int)*(uint32_t*)(f->esp + 4));
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
  return;
}

void halt()
{
  shutdown_power_off();
}

void exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  for(int i = 3; i <= 130; i++) if(thread_current()->file_descriptor[i] != NULL) close(i);
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
  if(pagedir_get_page(thread_current()->pagedir, buffer) == NULL) exit(-1);
  unsigned Count = 0;
  lock_acquire(&file_lock);
  if(!fd)
  {
    while(Count < length)
    {
      if((((char *)buffer)[Count] = input_getc()) == '\0')
        break;
      Count++;
    }
  }
  else if(3 <= fd && fd <= 130) 
  {
    if(thread_current()->file_descriptor[fd] == NULL)
    {
      lock_release(&file_lock);
      exit(-1);
    }
    Count=file_read(thread_current()->file_descriptor[fd], buffer, length);
  }
  else
  {
    lock_release(&file_lock);
    exit(-1);
  }
  lock_release(&file_lock);
  return Count;
}
int write (int fd, const void *buffer, unsigned length)
{
  if(pagedir_get_page(thread_current()->pagedir, buffer) == NULL) exit(-1);
  lock_acquire(&file_lock);
  int status = -1;
  if(fd == 1)
  {
    putbuf(buffer, length);
    status = length;
  }
  else if(3 <= fd && fd <= 130)
  {
    if(thread_current()->file_descriptor[fd] == NULL)
    {
      lock_release(&file_lock);
      exit(-1);
    }
    if(thread_current()->file_descriptor[fd]->deny_write) file_deny_write(thread_current()->file_descriptor[fd]);
    status = file_write(thread_current()->file_descriptor[fd], buffer, length);
  }
  else
  {
    lock_release(&file_lock);
    exit(-1);
  }
  lock_release(&file_lock);
  return status;
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
bool create (const char *file, unsigned initial_size)
{
  if(file == NULL || pagedir_get_page(thread_current()->pagedir, file) == NULL) exit(-1);
  return filesys_create(file, (off_t)initial_size);
}
bool remove (const char *file)
{
  if(file == NULL || pagedir_get_page(thread_current()->pagedir, file) == NULL) exit(-1);
  return filesys_remove(file);
}
int open (const char *file)
{
  int fd = -1;
  if(file == NULL || pagedir_get_page(thread_current()->pagedir, file) == NULL) exit(-1);
  lock_acquire(&file_lock);
  struct file* _file = filesys_open(file);
  if(_file != NULL)
  {
    for(int i = 3; i <= 130; i++)
    {
      if(thread_current()->file_descriptor[i] == NULL)
      {
        if(!strcmp(thread_current()->name, file)) file_deny_write(_file);
        thread_current()->file_descriptor[i] = _file;
        fd = i;
        break;
      }
    }
  }
  lock_release(&file_lock);
  return fd;
}
void close (int fd)
{
  if(thread_current()->file_descriptor[fd] == NULL) exit(-1);
  file_close(thread_current()->file_descriptor[fd]);
  thread_current()->file_descriptor[fd] = NULL;
}
int filesize (int fd)
{
  if(thread_current()->file_descriptor[fd] == NULL) exit(-1);
  return file_length(thread_current()->file_descriptor[fd]);
}
void seek (int fd, unsigned position)
{
  if(thread_current()->file_descriptor[fd] == NULL) exit(-1);
  file_seek(thread_current()->file_descriptor[fd], position);
  return;
}
unsigned tell (int fd)
{
  if(thread_current()->file_descriptor[fd] == NULL) exit(-1);
  return file_tell(thread_current()->file_descriptor[fd]);
}