#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

/* Opens a file for the given INODE, of which it takes ownership,
   and returns the new file.  Returns a null pointer if an
   allocation fails or if INODE is null. */
struct file *
file_open (struct inode *inode) 
{
  struct file *file = calloc (1, sizeof *file);
  if (inode != NULL && file != NULL)
    {
      file->inode = inode;
      file->pos = 0;
      file->deny_write = false;
      return file;
    }
  else
    {
      inode_close (inode);
      free (file);
      return NULL; 
    }
}

/* Opens and returns a new file for the same inode as FILE.
   Returns a null pointer if unsuccessful. */
struct file *
file_reopen (struct file *file) 
{
  return file_open (inode_reopen (file->inode));
}

/* Closes FILE. */
void
file_close (struct file *file) 
{
  if (file != NULL)
    {
      file_allow_write (file);
      inode_close (file->inode);
      free (file); 
    }
}

/* Returns the inode encapsulated by FILE. */
struct inode *
file_get_inode (struct file *file) 
{
  return file->inode;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at the file's current position.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   Advances FILE's position by the number of bytes read. */
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  off_t bytes_read = inode_read_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_read;
  return bytes_read;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   The file's current position is unaffected. */
off_t
file_read_at (struct file *file, void *buffer, off_t size, off_t file_ofs) 
{
  return inode_read_at (file->inode, buffer, size, file_ofs);
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at the file's current position.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   Advances FILE's position by the number of bytes read. */
off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_written;
  return bytes_written;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   The file's current position is unaffected. */
off_t
file_write_at (struct file *file, const void *buffer, off_t size,
               off_t file_ofs) 
{
  return inode_write_at (file->inode, buffer, size, file_ofs);
}

/* Prevents write operations on FILE's underlying inode
   until file_allow_write() is called or FILE is closed. */
void
file_deny_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (!file->deny_write) 
    {
      file->deny_write = true;
      inode_deny_write (file->inode);
    }
}

/* Re-enables write operations on FILE's underlying inode.
   (Writes might still be denied by some other file that has the
   same inode open.) */
void
file_allow_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (file->deny_write) 
    {
      file->deny_write = false;
      inode_allow_write (file->inode);
    }
}

/* Returns the size of FILE in bytes. */
off_t
file_length (struct file *file) 
{
  ASSERT (file != NULL);
  return inode_length (file->inode);
}

/* Sets the current position in FILE to NEW_POS bytes from the
   start of the file. */
void
file_seek (struct file *file, off_t new_pos)
{
  ASSERT (file != NULL);
  ASSERT (new_pos >= 0);
  file->pos = new_pos;
}

/* Returns the current position in FILE as a byte offset from the
   start of the file. */
off_t
file_tell (struct file *file) 
{
  ASSERT (file != NULL);
  return file->pos;
}

/* 현재 쓰레드의 fd를 초기화 */
void init_fileDescriptor(struct thread *currThread){
  for(int i=2; i<MAX_FILE_DESCRIPTOR; i++){
    currThread->fileDescriptor[i] = NULL;
  }
}

// /* 현재 쓰레드의 fd의 index에 해당 파일 삽입. fd가 음수면, 가장 작은 공간에 할당 fd 반환, 음수 반환은 실패 의미 */
int add_file_fileDescriptor(struct thread *currThread, struct file *f, int fd){
  if(fd == 0 || fd == 1 || fd >= MAX_FILE_DESCRIPTOR) 
    return -1;
  else if(fd == -1)
    if ((fd = find_space_fildDescriptor(currThread))== -1)
      return -1;
  
  currThread->fileDescriptor[fd] = f;
  return fd;
}

/* 현재 쓰레드의 fd 위치의 file 가져오기 */
struct file* get_file_fileDescriptor(struct thread *currThread, int fd){
  if(fd >= 2 && fd < MAX_FILE_DESCRIPTOR)
    return currThread->fileDescriptor[fd];
  else 
    return NULL;
}

/* 현재 쓰레드의 fd 위치의 file 닫기, fd가 음수인 경우 전체 닫기 */
void close_file_fileDescriptor(struct thread *currThread, int fd){
  int start = 2, end= MAX_FILE_DESCRIPTOR-1;

  if(fd == 0 || fd == 1){
    return;
  }else if(fd > 0){
    start = end = fd;
  }

  for(fd=start; fd <= end; fd++){
    file_close(currThread->fileDescriptor[fd]);
    currThread->fileDescriptor[fd] = NULL;
  }
}

/* 현재 쓰레드의 fd 중 가장 작은 공간 찾아서 반환. 공간이 없을 시 -1 반환 */
int find_space_fildDescriptor(struct thread *currThread){
  for(int i=2;i<MAX_FILE_DESCRIPTOR; i++){
    if(currThread->fileDescriptor[i] == NULL){
      return i;
    }
  }
  return -1;
}