#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"
#include "process.h"
void syscall_init (void);
 /* userprog/syscall.h */
void check_valid_address(const void *);
int wait (tid_t);
void halt (void);
void exit (int status);
tid_t exec (const char *file);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
int fibonacci(int a);
int max_of_four_int(int a, int b, int c, int d);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
void close (int fd);
int filesize (int fd);
void seek (int fd, unsigned position);
unsigned tell (int fd);
bool verify_stack (void *fault_addr, void *esp);
struct spte * check_address (void *addr, void *esp);
void check_valid_buffer (void *buffer, unsigned size, void *esp, bool to_write);
void check_valid_string (void *str, void *esp);
void check_valid_string_size (void *str, unsigned size, void *esp);
#endif