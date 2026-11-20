#include "idt.h"
#include "pic.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table

struct idt_entry idt[IDT_ENTRIES];

static struct idt_register idtr;

void idt_load() {
  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint32_t)idt;

  __asm__ volatile("lidt %0" : : "m"(idtr));
}

void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector,
                  uint8_t flags) {
  idt[num].offset_low = handler & 0xffff;
  idt[num].selector = selector;
  idt[num].zero = 0;
  idt[num].type_attributes = flags;
  idt[num].offset_high = (handler >> 16) & 0xffff;
}

extern void __isr0();
extern void __isr14();

extern void __irq0();
extern void __irq1();
extern void __irq14();
extern void __irq15();

extern void __test_irq14();

#define IRQ0_VECTOR PIC_OFFSET
#define IRQ1_VECTOR PIC_OFFSET + 1
#define IRQ14_VECTOR PIC_OFFSET + 14
#define IRQ15_VECTOR PIC_OFFSET + 15

#define INTERRUPT_GATE 0x8e
#define GDT_CODE 0x08 // 1(idx) * 8

#define EXCEPTION_DIV_0 0
#define EXCEPTION_PAGE_FAULT 14

void idt_init() {
  for (int i = 0; i < IDT_ENTRIES; i++) {
    idt_set_gate(i, 0, 0, 0);
  }

  idt_set_gate(EXCEPTION_DIV_0, (uint32_t)__isr0, GDT_CODE, INTERRUPT_GATE);
  idt_set_gate(EXCEPTION_PAGE_FAULT, (uint32_t)__isr14, GDT_CODE,
               INTERRUPT_GATE);

  idt_set_gate(IRQ0_VECTOR, (uint32_t)__irq0, GDT_CODE, INTERRUPT_GATE);
  idt_set_gate(IRQ1_VECTOR, (uint32_t)__irq1, GDT_CODE, INTERRUPT_GATE);
  idt_set_gate(IRQ14_VECTOR, (uint32_t)__irq14, GDT_CODE, INTERRUPT_GATE);
  idt_set_gate(IRQ15_VECTOR, (uint32_t)__irq15, GDT_CODE, INTERRUPT_GATE);

  idt_load();
}
