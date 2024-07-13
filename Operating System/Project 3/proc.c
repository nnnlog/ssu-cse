#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define min(a, b) ((a) < (b) ? (a) : (b))


struct node {
  struct proc *proc;
  struct node *next, *prev;
};

struct linked_list {
  struct node *front, *back;
};

#define NULL (0)

struct node _node[NPROC * 2];
int _use[NPROC * 2];

struct node *create_node() {
  for (int i = 0; i < NPROC * 2; i++) {
    if (!_use[i]) {
      _use[i] = 1;
      memset(_node + i, 0, sizeof(struct node));
      return _node + i;
    }
  }
  return 0;
}

void free_node(struct node *node) {
  _use[node - _node] = 0;
  node->prev = node->next = NULL;
}

void linked_list_push_back(struct linked_list *container, struct node *node) {
  node->prev = node->next = NULL;
  if (container->front == NULL) {
    container->front = container->back = node;
  } else {
    node->prev = container->back;
    container->back->next = node;

    container->back = node;
  }
}

struct node *linked_list_shift(struct linked_list *container) {
  if (container->front == NULL) return NULL;
  struct node *ret = container->front;

  container->front = container->front->next;
  if (container->front == NULL) container->back = NULL;
  else container->front->prev = NULL;

  ret->next = ret->prev = NULL;

  return ret;
}

struct node *linked_list_pop(struct linked_list *container) {
  if (container->back == NULL) return NULL;
  struct node *ret = container->back;
  ret->next = ret->prev = NULL;

  container->back = container->back->prev;
  if (container->back == NULL) container->front = NULL;
  else container->back->next = NULL;
  return ret;
}


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// ptable must be held.
static int min_priority() {
  struct proc *p;
  int min_priority = 100;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if ((p->state == RUNNABLE || p->state == RUNNING) && p->pid >= 3) min_priority = min(min_priority, p->priority);

  if (min_priority == 100) min_priority = 0;
  return min_priority;
}


//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p->priority = min_priority(); // 우선 순위
  p->tq_tick = 0; // Time Quantum
  p->proc_tick = 0; // process tick
  p->cpu_used = 0; // total used tick
  p->exit_tick = -1; // exit tick

  p->for_test = 0; // for test? (set 1 when call set_sched_info)
  p->resp_tick = 0; // response tick
  p->resp_ok = 0;
  p->turn_arnd_tick = 0; // turn around tick

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

void update_priority_value() {
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state != RUNNABLE) continue;
    p->priority = min(99, p->priority + p->priority_tick / 10);
    p->priority_tick = 0;
  }
}

void update_queue(struct linked_list queue[25]) {
  struct proc *p;

  for (int i = 0; i < NPROC; i++) {
    p = ptable.proc + i;
    if (p->pid <= 2) p->priority = 99;
  }
  int exist_process[NPROC] = {0};
  for (int i = 0; i < 25; i++) {
    struct node *curr = queue[i].front;
    struct linked_list list = {0};
    while (curr) {
      struct node *tmp = curr->next;

      p = curr->proc;

      if (p->state != RUNNABLE || i != p->priority / 4) { // 프로세스 정보가 불일치하면 큐에서 제거함
        free_node(curr);
      } else {
        if (exist_process[p - ptable.proc]) panic("scheduler process duplicated");
        exist_process[p - ptable.proc] = 1;
        linked_list_push_back(&list, curr);
//          if (p->for_test)
//            cprintf("rechk: %d %d %d %d %d\n", p->pid, p->state, p->resp_ok, p->priority, p->turn_arnd_tick);
      }

      curr = tmp;
    }
    queue[i].front = list.front;
    queue[i].back = list.back;
  }

  for (int i = 0; i < NPROC; i++) {
    p = ptable.proc + i;
    if (!exist_process[i] && p->state == RUNNABLE) {
      struct node *node = create_node();
      node->proc = p;
      linked_list_push_back(&queue[p->priority / 4], node); // 큐에 안 들어있는 프로세스는 큐에 넣어줌
//        if (p->for_test)
//          cprintf("insert: %d %d %d %d %d\n", p->pid, p->state, p->resp_ok, p->priority, p->turn_arnd_tick);
    }
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void) {
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  struct linked_list queue[25] = {0};

  // Enable interrupts on this processor.
  sti();

  for (/*int t = 0*/;;) {
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    // ptable.proc에 있는 process struct address는 바뀌지 않는다.
    // UNUSED에서 풀린 process 혹은 UNUSED가 된 process는 큐에서 어떻게 해야 할까?

    // 1. 큐 돌면서 프로세스 정보와 우선순위 정보가 일치하지 않거나, RUNNABLE 상태가 아니면 큐에서 제거함
    // 2. 프로세스가 있어야 할 큐에 없으면 큐에 넣음
    // 3. 최상위 우선순위 레벨의 큐에서 프로세스 하나 뽑음

//    // update priority
//    update_priority_value();
    update_queue(queue);

    for (int i = 0; i < 25; i++) {
      if (queue[i].front) {
        p = queue[i].front->proc;

//#ifdef DEBUG
//        cprintf("(befr) PID : %d, priority : %d, proc_tick : %d ticks, priority_tick : %d ticks, total_cpu_usage : %d ticks\n", p->pid, p->priority, p->proc_tick, p->priority_tick, p->cpu_used);
//#endif

//        p->priority_tick += p->proc_tick; // lazy_proc_tick이 priority_tick과 같음
        p->proc_tick = 0; // 현재 스케쥴링되어 사용한 프로세스 틱
        p->tq_tick = 30; // time quantum tick
//        p->tq_tick = min(30, 60 - t); // time quantum tick

        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&c->scheduler, p->context);
        switchkvm();

#ifdef DEBUG
        if (p->for_test)
          cprintf("PID : %d, priority : %d, proc_tick : %d ticks, priority_tick : %d ticks, total_cpu_usage : %d ticks\n", p->pid, p->priority, p->proc_tick, p->priority_tick, p->cpu_used);
#endif

        if (p->proc_tick && p->for_test) {
          p->resp_ok = 1;
        }

//        t += p->proc_tick;
        linked_list_push_back(&queue[i], linked_list_shift(&queue[i])); // 현재 큐에서 빼고 큐 제일 뒤로 넣음

        if (p->for_test) {
//          cprintf("P: %d %d %d %d %d\n", p->pid, p->state, p->proc_tick, p->priority, p->turn_arnd_tick);
          for (struct proc *j = ptable.proc; j < ptable.proc + NPROC; j++)
            if (j->state == RUNNABLE || j == p) {
              j->turn_arnd_tick += p->proc_tick;
//              cprintf("C: %d %d %d %d %d\n", j->pid, j->state, j->resp_ok, j->priority, j->turn_arnd_tick);
//              cprintf("%x\n", queue[j->priority / 4].front);
              if (!j->resp_ok) j->resp_tick += p->proc_tick;
            }

          if (p->exit_tick == 0) {
#ifdef EVAL
            cprintf("%d\t%d\n", myproc()->turn_arnd_tick, myproc()->resp_tick);
#else
            cprintf("PID : %d terminated (turn around tick : %d, resp tick : %d)\n", myproc()->pid, myproc()->turn_arnd_tick, myproc()->resp_tick);
#endif
          }
        }

        c->proc = 0;
        break;
      }
    }

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      p->priority = min_priority();
      p->proc_tick = 0;
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING) {
        p->state = RUNNABLE;
        p->proc_tick = 0;
        p->priority = min_priority();
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
