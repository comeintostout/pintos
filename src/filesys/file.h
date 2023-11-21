#ifndef FILESYS_FILE_H
#define FILESYS_FILE_H

#include "filesys/off_t.h"
#include "threads/thread.h"

struct inode;

/* Opening and closing files. */
struct file *file_open (struct inode *);
struct file *file_reopen (struct file *);
void file_close (struct file *);
struct inode *file_get_inode (struct file *);

/* Reading and writing. */
off_t file_read (struct file *, void *, off_t);
off_t file_read_at (struct file *, void *, off_t size, off_t start);
off_t file_write (struct file *, const void *, off_t);
off_t file_write_at (struct file *, const void *, off_t size, off_t start);

/* Preventing writes. */
void file_deny_write (struct file *);
void file_allow_write (struct file *);

/* File position. */
void file_seek (struct file *, off_t);
off_t file_tell (struct file *);
off_t file_length (struct file *);

////// file descriptor 관련 함수
void init_fileDescriptor(struct thread *currThread);
int add_file_fileDescriptor(struct thread *currThread, struct file *f, int index);
struct file* get_file_fileDescriptor(struct thread *currThread, int fd);
void close_file_fileDescriptor(struct thread *currThread, int fd);
int find_space_fildDescriptor(struct thread *currThread);
bool validateFdRange(int fd, int lowCut, int highCut);
bool validateFileNameContraints(const char* fileName);


#endif /* filesys/file.h */
