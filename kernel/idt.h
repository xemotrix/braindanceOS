#ifndef IDT_H
#define IDT_H
// INTERRUPT DESCRIPTOR TABLE

#include <stdint.h>

struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t type_attributes;
  uint16_t offset_high;
} __attribute__((packed));

#define IDT_ENTRIES 256

extern struct idt_entry idt[IDT_ENTRIES];

struct idt_register {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

void idt_load();
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector,
                  uint8_t flags);

void idt_init();

#endif
