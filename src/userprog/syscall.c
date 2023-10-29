#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void halt(){
  shutdown_power_off();
}

void exit(int status){
  struct thread *currThread = thread_current();
  printf("%s: exit(%d)\n", currThread->name, status);
  currThread->exitStatus = status;
  thread_exit();
}

int exec(const char *cmd_line){
  return process_execute(cmd_line);
  // int childTid = process_execute(cmd_line);
  // if(childTid != -1){
  //   struct thread* currThread = thread_current();
  //   struct thread* child;

  //   // 현재 쓰레드의 자식 list들을 순회. 해당 elem을 통해 list_entry()로 thread를 가져옴. 
  //   for(struct list_elem* e=list_begin(&(currThread->childList)); e!=list_end(&(currThread->childList)) ; e=list_next(e)){
  //     child = list_entry(e, struct thread, childElem);
  //     if(childTid == child->tid){
  //       sema_down(&(child->sema_load)); // isFinished가 down되며, 자식 쓰레드가 죽으면서 up 할 때 깨어남
  //       break;
  //     }
  //   }
  // }
  // return childTid;
}

int wait (int pid){
  return process_wait(pid);
}

bool create(const char* file, unsigned int init_size){
  validateFileNameContraints(file);
  return filesys_create(file, init_size);
}

bool remove(const char* file){
  validateFileNameContraints(file);
  return filesys_remove(file);
}

int open(const char *file){
  int fd = -1;
  validateFileNameContraints(file);
  
  lock_acquire(&filesys_lock);
  // 파일을 여는 작업
  struct file *f = filesys_open(file);
  if(f != NULL)
    fd = add_file_fileDescriptor(thread_current(), f, -1);

  lock_release(&filesys_lock);
  return fd;
}

int filesize(int fd){
  if(!validateFdRange(fd,2,MAX_FILE_DESCRIPTOR))
    return 0;
  struct file *f = get_file_fileDescriptor(thread_current(),fd);
  if(f == NULL) 
    exit(-1);
  return file_length(f);
}

int read(int fd, void *buffer, unsigned int size){
  int readSize = 0; 
  uint8_t ch;
  if(!validateFdRange(fd, 0, MAX_FILE_DESCRIPTOR) || fd == 1) exit(-1);
  validateAddress(buffer);

  lock_acquire(&filesys_lock);
  if(fd >= 2){
    struct file* f = get_file_fileDescriptor(thread_current(),fd);
    if(f == NULL){
      lock_release(&filesys_lock);
      exit(-1);
    }
    readSize = file_read(f,buffer, size);
  } else{
    for(readSize=0;(readSize<size);readSize++){
      if(!(ch = input_getc()))
        break;
      ((char*)buffer)[readSize] = (char)(ch);
    }
  }
  lock_release(&filesys_lock);
  return readSize;
}

int write(int fd, void *buffer, unsigned int size){
  int writeSize = 0;
  if(!validateFdRange(fd, 1, MAX_FILE_DESCRIPTOR)) exit(-1);
  validateAddress(buffer);

  lock_acquire(&filesys_lock);
  if(fd == 1){
    putbuf(buffer, size);
    writeSize = size;
  } else {
    struct file* f = get_file_fileDescriptor(thread_current(), fd);
    if(f == NULL){
      lock_release(&filesys_lock);
      exit(-1);
    }
    writeSize = file_write(f,buffer,size);
  }

  lock_release(&filesys_lock);
  return writeSize;
}

void seek(int fd, unsigned int position){
  if(!validateFdRange(fd,2,MAX_FILE_DESCRIPTOR)) exit(-1);
  struct file *f = get_file_fileDescriptor(thread_current(),fd);
  if(f == NULL) exit(-1);
  file_seek(f,fd);
}

unsigned int tell(int fd){  
  if(!validateFdRange(fd,2,MAX_FILE_DESCRIPTOR))exit(-1);
  
  struct file *f = get_file_fileDescriptor(thread_current(),fd);
  if(f == NULL) exit(-1);
  return file_tell(f);
}


void close(int fd){
  if(!validateFdRange(fd, 2, MAX_FILE_DESCRIPTOR))exit(-1);
  close_file_fileDescriptor(thread_current(), fd);
}


///// 해당 addr 이 valid 하지 않다면 exit 해버리기
void validateAddress(const void *addr){
  if(addr == NULL || !is_user_vaddr(addr))
    exit(-1);
}
///// validate 할 address가 여러개 일 때
void validateAddressList(const void *startAddr,unsigned int addressCount){
  for (int i=0; i<addressCount; i++){
    validateAddress(startAddr + i);
  }
}

void validateFileNameContraints(const char* fileName){
  if(fileName == NULL)
    exit(-1);
}

/** lowCut <= fd < highCut  : 1, else : 0*/
bool validateFdRange(int fd, int lowCut, int highCut){
  if(fd >= lowCut && fd < highCut)
    return 1;
  return 0;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int* stackPointer = (int*)(f->esp);
  int syscallNumber = *(int*)(f->esp);

  switch(syscallNumber){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      validateAddress(stackPointer+1);
      exit((int)*(stackPointer + 1));
      break;
    case SYS_EXEC:
      validateAddress(stackPointer+1);
      f->eax = exec((char*)*(stackPointer + 1));
      break;
    case SYS_WAIT:
      validateAddress(stackPointer+1);
      f->eax = wait((int)*(stackPointer + 1));
      break;
    case SYS_CREATE:
      validateAddressList(stackPointer+1,2);
      f->eax = create((char*)*(stackPointer + 1), (unsigned int)*(stackPointer + 2));
      break;    
    case SYS_REMOVE:
      validateAddress(stackPointer+1);
      f->eax = remove((char*)*(stackPointer + 1));
      break;
    case SYS_OPEN:
      validateAddress(stackPointer+1);
      f->eax = open((char*)*(stackPointer + 1));
      break;
    case SYS_FILESIZE:
      validateAddress(stackPointer+1);
      f->eax = filesize((int)*(stackPointer + 1));
      break;
    case SYS_READ:
      validateAddressList(stackPointer+1,3);
	    f->eax = read((int)*(stackPointer + 1), (void*)*(stackPointer + 2), (unsigned int)*(stackPointer + 3));
      break;
    case SYS_WRITE:
      validateAddressList(stackPointer+1,3);
      f->eax = write((int)*(stackPointer + 1), (void*)*(stackPointer + 2), (unsigned int)*(stackPointer + 3));
      break;
    case SYS_SEEK:
      validateAddressList(stackPointer+1,2);
      seek((int)*(stackPointer + 1), (unsigned int)*(stackPointer + 2));
      break;          
    case SYS_TELL:
      validateAddress(stackPointer+1);
      f->eax = tell((int)*(stackPointer + 1));
      break;    
    case SYS_CLOSE:
      validateAddress(stackPointer+1);
      close((int)*(stackPointer + 1));
      break;
    default:
      exit(-1);
  }
}
