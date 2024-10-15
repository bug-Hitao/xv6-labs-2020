// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char *ref_page;
  int page_cnt;
  char *end_;
} kmem;


int
pagecnt(void *pa_start, void *pa_end){
  char *p;
  int cnt = 0;
  p = (char *) PGROUNDUP((uint64) pa_start);
  for(; p + PGSIZE <= (char *) pa_end; p += PGSIZE)
    cnt++;
  return cnt;
}


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.page_cnt = pagecnt(end, (void *) PHYSTOP);
  printf("page_cnt: %d\n", kmem.page_cnt);

  kmem.ref_page = end;  //将end(内核代码截止)地址分配给ref_page，ref_page将后续page_cnt个字节作为数组用来统计引用数。
  for (int i = 0; i < kmem.page_cnt; ++i){
    kmem.ref_page[i] = 0;
  }

  kmem.end_ = kmem.ref_page + kmem.page_cnt;  //指针算术运算，将kmem.ref_page地址向前移动page_cnt个sizeof(char)
   

  freerange(kmem.end_, (void*) PHYSTOP);
}

int
page_index(uint64 pa){
  pa = PGROUNDDOWN( pa);
  int res = (pa - (uint64) kmem.end_) / PGSIZE;
  if(res < 0 || res >= kmem.page_cnt){
    printf("res: %d, pa: %p, kmem.end_: %p\n", res, pa, kmem.end_);
    panic("page_index illegal");
  }
  return res;
}

void
incr(void* pa){
  int index = page_index((uint64) pa);
  acquire(&kmem.lock);
  kmem.ref_page[index]++;
  release(&kmem.lock);
}

void
desc(void* pa){
  int index = page_index((uint64) pa);
  acquire(&kmem.lock);
  kmem.ref_page[index]--;
  release(&kmem.lock);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  int index = page_index((uint64)pa);
  if(kmem.ref_page[index] > 1){
    desc(pa);
    return ;
  }
  if (kmem.ref_page[index] == 1){
    desc(pa);
  }
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void
printf_free_pages_cnt(){
  struct run *r;
  int cnt = 0;
  acquire(&kmem.lock);
  r = kmem.freelist;
  while(r){
    cnt++;
    r = r->next;
  }
  release(&kmem.lock);
  printf("free_pages_cnt: %d\n", cnt);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    //printf_free_pages_cnt();
    incr((void*) r); //分配r页面，所以r页面引用加1
  }
    
  return (void*)r;
}
