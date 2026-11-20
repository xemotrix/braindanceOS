#include "irq.h"
#include "../drivers/ata.h"
#include "../drivers/keyboard.h"
#include "../drivers/ports.h"
#include "../kernel/panic.h"
#include "../kernel/pic.h"
#include "../kernel/scheduler.h"

#define PIT_IRQ 0
#define KEYBOARD_IRQ 1
#define KEYBOARD_IRQ 1

void irq14_handler(void) {
  ata_irq_handler();
  PIC_send_EOI(14);
}

void irq15_handler(void) { PANIC("irq15 handler!\n"); }

void irq1_handler(void) {
  // read scan code from keyboard controller;
  uint8_t scancode = inb(0x60);

  handle_keyboard_interrupt(scancode);

  // tell the master pic that we handled the interrupt;
  PIC_send_EOI(KEYBOARD_IRQ);
}

struct interrupt_frame *irq0_handler(struct interrupt_frame *frame) {
  struct interrupt_frame *new_frame = scheduler_PIT_IRQ_handler(frame);
  PIC_send_EOI(PIT_IRQ);
  return new_frame;
}
