#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

// disable interrupts
#define CLI __asm__ volatile("cli");
// enable interrupts
#define STI __asm__ volatile("sti");
// halt
#define HLT __asm__ volatile("hlt");

struct interrupt_frame {
  // by pushad
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp; // popad doesn't reset esp, so doesn't matter what's here
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  // interrupt leaves these on the stack
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
} __attribute__((packed));

#endif
