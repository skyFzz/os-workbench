// Modified xv6 Sleeping locks

#include <common.h>
#define UNLOCKED 0
#define LOCKED 1

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct cpu *cpu = &cpus[cpu_current];
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

void
initsleeplock(struct sleeplock *lk, char *name)
{
  spin_init(&lk->lk, "sleep lock");
  lk->name = name;
  lk->status = UNLOCKED;
  lk->cpu = NULL;
}

void
acquiresleep(struct sleeplock *lk)
{
  spin_lock(&lk->lk);
  while (lk->status) {
    sleep(lk, &lk->lk);
  }
  lk->locked = LOCKED;
  lk->cpu = &cpus[cpu_current()];
  spin_unlock(&lk->lk);
}

void
releasesleep(struct sleeplock *lk)
{
  spin_lock(&lk->lk);
  lk->locked = UNLOCKED;
  lk->cpu = NULL;
  wakeup(lk);
  spin_unlock(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  spin_lock(&lk->lk);
  r = lk->status && (lk->cpu == &cpus[cpu_current()]);
  spin_unlock(&lk->lk);
  return r;
}
