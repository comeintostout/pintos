#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>

struct lock filesys_lock;

void syscall_init (void);

void halt();
void exit(int status);
int exec(const char *cmd_line);
int wait (int pid);
bool create(const char* file, unsigned int init_size);
bool remove(const char* file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned int size);
int write(int fd, void *buffer, unsigned int size);
void seek(int fd, unsigned int position);
unsigned int tell(int fd);
void close(int fd);
bool validateFdRange(int fd, int lowCut, int highCut);

#endif /* userprog/syscall.h */
