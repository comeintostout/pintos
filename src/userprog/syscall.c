#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
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
}

int wait (int pid){
  return process_wait(pid);
}

int open(const char *file){
  if(file == NULL || strlen(file) <=0 || strlen(file) > 14)
    return -1;

  struct thread *currThread = thread_current();
  int fd = -1;
  
  // 파일을 여는 작업
  struct file *f = filesys_open(file);
  if(f != NULL)
    fd = add_file_fileDescriptor(currThread, f, -1);

  return fd;
}

unsigned int read(int fd, const void *buffer, unsigned int size){
  unsigned int readSize = 0;
  switch (fd)
  {
    case 0:
      for(readSize; readSize<size; readSize++){
        uint8_t ch = input_getc();
        ((char*)buffer)[readSize] = (char)(ch);
        if(ch == '\n' || ch == '\0')
          break;
      }
      return readSize;
  }
  return -1;
}

unsigned int write(int fd, const void *buffer, unsigned int size){
  switch (fd)
  {
    case 1:
      putbuf(buffer, size);
      return size;
  }
  return -1;
}

///// 해당 addr 이 valid 하지 않다면 exit 해버리기
void validateAddress(const void *addr){
  if(addr == NULL || !is_user_vaddr(addr)){
    exit(-1);
  }
}
///// validate 할 address가 여러개 일 때
void validateAddressList(const void *startAddr,unsigned int addressCount){
  for (int i=0; i<addressCount; i++){
    validateAddress(startAddr + i);
  }
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
    case SYS_OPEN:
      validateAddress(stackPointer+1);
      f->eax = open((int)*(stackPointer + 1));
      break;
    case SYS_READ:
      validateAddressList(stackPointer+1,3);
	    f->eax = read((int)*(stackPointer + 1), (void*)*(stackPointer + 2), (unsigned int)*(stackPointer + 3));
      break;
    case SYS_WRITE:
      validateAddressList(stackPointer+1,3);
      f->eax = write((int)*(stackPointer + 1), (void*)*(stackPointer + 2), (unsigned int)*(stackPointer + 3));
      break;
    default:
      exit(-1);
  }
}
