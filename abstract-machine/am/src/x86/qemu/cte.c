#include "x86-qemu.h"
#include <klib.h>

// function pointer
static Context* (*user_handler)(Event, Context*) = NULL;
#if __x86_64__
// idt: tells the CPU where to jump when interrupts/exceptions occur;
//      contains NR_IRQ entries (number of interrupt requests)
// x86-qemu.h:13:#define NR_IRQ         256     // IDT size
static GateDesc64 idt[NR_IRQ];
#define GATE GATE64
#else
static GateDesc32 idt[NR_IRQ];
#define GATE GATE32
#endif

#define IRQHANDLE_DECL(id, dpl, err) \
  void __am_irq##id();

IRQS(IRQHANDLE_DECL)
void __am_irqall();
void __am_kcontext_start();

/*
  /home/hz/Laser/os-workbench-copy/abstract-machine/am/src/x86/qemu/x86-qemu.h
  #if __x86_64__
struct trap_frame {
  Context saved_context;
  uint64_t irq, errcode;  // irq is saved by us, representing which trap
  uint64_t rip, cs, rflags, rsp, ss;  // x86 hardware saves these
};

  /home/hz/Laser/os-workbench-copy/abstract-machine/am/include/arch/x86_64-qemu.h
 struct Context {
  void    *cr3;
  uint64_t rax, rbx, rcx, rdx,
           rbp, rsi, rdi,
           r8, r9, r10, r11,
           r12, r13, r14, r15,
           rip, cs, rflags,
           rsp, ss, rsp0;
};
*/
void __am_irq_handle(struct trap_frame *tf) {
  Context *saved_ctx = &tf->saved_context;
  Event ev = {
    .event = EVENT_NULL,
    .cause = 0, .ref = 0,
    .msg = "(no message)",
  };

// store the registers that are saved by CPU too 
#if __x86_64
  saved_ctx->rip    = tf->rip;
  saved_ctx->cs     = tf->cs;
  saved_ctx->rflags = tf->rflags;
  saved_ctx->rsp    = tf->rsp;
  saved_ctx->rsp0   = CPU->tss.rsp0;
  saved_ctx->ss     = tf->ss;
#else
  saved_ctx->eip    = tf->eip;
  saved_ctx->cs     = tf->cs;
  saved_ctx->eflags = tf->eflags;
  saved_ctx->esp0   = CPU->tss.esp0;
  saved_ctx->ss3    = USEL(SEG_UDATA);
  // no ss/esp saved for DPL_KERNEL
  saved_ctx->esp = (tf->cs & DPL_USER ? tf->esp : (uint32_t)(tf + 1) - 8);
#endif
  // acquire the based address of the page
  saved_ctx->cr3    = (void *)get_cr3();

  #define IRQ    T_IRQ0 +
  #define MSG(m) ev.msg = m;

  if (IRQ 0 <= tf->irq && tf->irq < IRQ 32) {
    __am_lapic_eoi();   // Signal completion to the APIC
  }

  // examine the interrupt number saved
  switch (tf->irq) {
    case IRQ 0: MSG("timer interrupt (lapic)")
      ev.event = EVENT_IRQ_TIMER; break;
    case IRQ 1: MSG("I/O device IRQ1 (keyboard)")
      ev.event = EVENT_IRQ_IODEV; break;
    case IRQ 4: MSG("I/O device IRQ4 (COM1)")
      ev.event = EVENT_IRQ_IODEV; break;
    case EX_SYSCALL: MSG("int $0x80 system call")
      ev.event = EVENT_SYSCALL; break;
    // am/src/x86/x86.h:51:#define EX_YIELD       0x81
    case EX_YIELD: MSG("int $0x81 yield")
      ev.event = EVENT_YIELD; break;
    case EX_DE: MSG("DE #0 divide by zero")
      ev.event = EVENT_ERROR; break;
    case EX_UD: MSG("UD #6 invalid opcode")
      ev.event = EVENT_ERROR; break;
    case EX_NM: MSG("NM #7 coprocessor error")
      ev.event = EVENT_ERROR; break;
    case EX_DF: MSG("DF #8 double fault")
      ev.event = EVENT_ERROR; break;
    case EX_TS: MSG("TS #10 invalid TSS")
      ev.event = EVENT_ERROR; break;
    case EX_NP: MSG("NP #11 segment/gate not present")
      ev.event = EVENT_ERROR; break;
    case EX_SS: MSG("SS #12 stack fault")
      ev.event = EVENT_ERROR; break;
    case EX_GP: MSG("GP #13, general protection fault")
      ev.event = EVENT_ERROR; break;
    case EX_PF: MSG("PF #14, page fault, @cause: PROT_XXX")
      ev.event = EVENT_PAGEFAULT;
      if (tf->errcode & 0x1) ev.cause |= MMAP_NONE;
      if (tf->errcode & 0x2) ev.cause |= MMAP_WRITE;
      else                   ev.cause |= MMAP_READ;
      ev.ref = get_cr2();
      break;
    default: MSG("unrecognized interrupt/exception")
      ev.event = EVENT_ERROR;
      ev.cause = tf->errcode;
      break;
  }

  // return the context of the next scheduled thread
  Context *ret_ctx = user_handler(ev, saved_ctx);
  /*
  printf("ret_ctx content:\n");
  printf("\tcr3: 0x%x\n", ret_ctx->cr3);
  printf("\trdi (arg): 0x%x\n", ret_ctx->rdi);
  printf("\trsi (entry): 0x%x\n", ret_ctx->rsi);
  printf("\trsp (area.end): 0x%x\n", ret_ctx->rsp);
  */
  panic_on(!ret_ctx, "returning to NULL context");

  if (ret_ctx->cr3) {
    set_cr3(ret_ctx->cr3);
#if __x86_64__
    CPU->tss.rsp0 = ret_ctx->rsp0;
#else
    CPU->tss.ss0  = KSEL(SEG_KDATA);
    CPU->tss.esp0 = ret_ctx->esp0;
#endif
  }

  printf("Start poping...\n");
  __am_iret(ret_ctx);
}

bool cte_init(Context *(*handler)(Event, Context *)) {
  panic_on(cpu_current() != 0, "init CTE in non-bootstrap CPU");
  panic_on(!handler, "no interrupt handler");

  // The loop initializes the Interrupt Descriptor Table (IDT)
  for (int i = 0; i < NR_IRQ; i ++) {
/*
  src/x86/x86.h

12 typedef struct {
 11   uint32_t off_15_0  : 16;
 10   uint32_t cs        : 16;
  9   uint32_t isv       :  3;
  8   uint32_t zero1     :  5;
  7   uint32_t type      :  4;
  6   uint32_t zero2     :  1;
  5   uint32_t dpl       :  2;
  4   uint32_t p         :  1;
  3   uint32_t off_31_16 : 16;
  2   uint32_t off_63_32 : 32;
  1   uint32_t rsv       : 32;
145 } GateDesc64;

 1 #define GATE64(type, cs, entry, dpl) (GateDesc64)             \
216   { (uint64_t)(entry) & 0xffff, (cs), 0, 0, (type), 0, (dpl), \
  1     1, ((uint64_t)(entry) >> 16) & 0xffff, (uint64_t)(entry) >> 32, 0 }
  2 

    STS_TG: Trap Gate type
    KSEL(SEG_KCODE): Kernel code segment selector
    __am_irqall: init every IDT entry to point to __am_irqall (a generic int handler)
                ensures no unhandled interrupts crash the system
    DPL_KERN: Descriptor Privilege Level
*/
    idt[i]  = GATE(STS_TG, KSEL(SEG_KCODE), __am_irqall,  DPL_KERN);
  }

// Below is a multi-stage macro
/*
  /home/hz/Laser/os-workbench-copy/abstract-machine/am/src/trap64.S
  #define NOERR     push $0
#define ERR
#define IRQ_DEF(id, dpl, err) \
  .globl __am_irq##id; __am_irq##id: cli;      err; push $id; jmp trap;
IRQS(IRQ_DEF)
  .globl  __am_irqall;  __am_irqall: cli; push $0; push $-1; jmp trap;
*/
#define IDT_ENTRY(id, dpl, err) \
    idt[id] = GATE(STS_TG, KSEL(SEG_KCODE), __am_irq##id, DPL_##dpl);
/*
  // Override specific entries with dedicated handlers

  /home/hz/Laser/os-workbench-copy/abstract-machine/am/src/x86/x86.h
  // Interrupt descriptor configuration 
  // (interrupt_number, privilege_level, hardware errorcode)
#define IRQS(_) \
  _(  0, KERN, NOERR) \
  _(  1, KERN, NOERR) \
  _(  2, KERN, NOERR) \
  _(  3, KERN, NOERR) \
  _(  4, KERN, NOERR) \
  _(  5, KERN, NOERR) \
  _(  6, KERN, NOERR) \
  _(  7, KERN, NOERR) \
  _(  8, KERN,   ERR) \
  _(  9, KERN, NOERR) \
  _( 10, KERN,   ERR) \
  _( 11, KERN,   ERR) \
  _( 12, KERN,   ERR) \
  _( 13, KERN,   ERR) \
  _( 14, KERN,   ERR) \
  _( 15, KERN, NOERR) \
  _( 16, KERN, NOERR) \
  _( 19, KERN, NOERR) \
  _( 31, KERN, NOERR) \
  _( 32, KERN, NOERR) \
  _( 33, KERN, NOERR) \
  _( 34, KERN, NOERR) \
  _( 35, KERN, NOERR) \
  _( 36, KERN, NOERR) \
  _( 37, KERN, NOERR) \
  _( 38, KERN, NOERR) \
  _( 39, KERN, NOERR) \
  _( 40, KERN, NOERR) \
  _( 41, KERN, NOERR) \
  _( 42, KERN, NOERR) \
  _( 43, KERN, NOERR) \
  _( 44, KERN, NOERR) \
  _( 45, KERN, NOERR) \
  _( 46, KERN, NOERR) \
  _( 47, KERN, NOERR) \
  _(128, USER, NOERR) \
  _(129, USER, NOERR)
*/
  IRQS(IDT_ENTRY)

  // init with a function pointer
  // cte_init(os->trap); in main.c
  // cte_init(on_interrupt); in thread.os
  user_handler = handler;
  return true;
}

void yield() {
  /*
    /home/hz/Laser/os-workbench-copy/abstract-machine/am/src/x86/x86.h
    1 #define interrupt(id) \
    2   asm volatile ("int $" #id);

    The int 0x81 instruction is a software-generated interrupt on x86 systems, but its specific behavior depends entirely on how the operating system configures interrupt 0x81 in the Interrupt Descriptor Table (IDT). Here's what you need to know:
      If the OS hasn't set up an entry for interrupt 0x81 in the IDT:
      The CPU will trigger a general protection fault (#GP) or double fault, crashing the system.
      This happens because unconfigured interrupts are treated as errors.
  */
  interrupt(0x81);
}

bool ienabled() {
  return (get_efl() & FL_IF) != 0;
}

void iset(bool enable) {
  if (enable) sti();
  else cli();
}

void __am_panic_on_return() { panic("kernel context returns"); }

// 创建内核态运行的上下文
Context* kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *ctx = kstack.end - sizeof(Context);
  // printf("ctx starts at: %p\n", ctx);
  *ctx = (Context) { 0 };

#if __x86_64__
  ctx->cs     = KSEL(SEG_KCODE);
  ctx->rip    = (uintptr_t)__am_kcontext_start;
  ctx->rflags = FL_IF;
  ctx->rsp    = (uintptr_t)kstack.end;
#else
  ctx->ds     = KSEL(SEG_KDATA);
  ctx->cs     = KSEL(SEG_KCODE);
  ctx->eip    = (uintptr_t)__am_kcontext_start;
  ctx->eflags = FL_IF;
  ctx->esp    = (uintptr_t)kstack.end;
#endif

  ctx->GPR1 = (uintptr_t)arg;
  ctx->GPR2 = (uintptr_t)entry;

  return ctx;
}

void __am_percpu_initirq() {
  __am_ioapic_enable(IRQ_KBD, 0);
  __am_ioapic_enable(IRQ_COM1, 0);
  set_idt(idt, sizeof(idt));
}
