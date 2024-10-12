#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

extern uint64 acquire_freemem();
extern uint64 acquire_nproc();

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//在kernel/sysproc.c中添加一个sys_trace()函数
uint64
sys_trace(void)
{
  int mask;
  if(argint(0, &mask) < 0)  //将陷阱帧中a0寄存器的值存入mask中，a0存放的是系统调用的第一个参数，在trace中第一个参数为整数“掩码”
    return -1;
  
  struct proc *p = myproc();
  p->trace_mask = mask;
  
  return 0;  
}

uint64
sys_sysinfo(void)
{
  struct sysinfo info;
  uint64 addr;
  struct proc *p = myproc();

  info.freemem = acquire_freemem();
  info.nproc = acquire_nproc();

  //将陷阱帧a0寄存器对应的值存入addr中，a0存放的是系统调用的第一个参数--指向struct sysinfo的指针
  if(argaddr(0, &addr) < 0)
    return -1;

  //将 info 结构体的内容从内核空间复制到用户进程的虚拟地址空间 addr 对应的内存位置
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
    return -1;

  
  return 0;
}