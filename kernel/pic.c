#include "pic.h"
#include "../drivers/ports.h"
// PROGRAMMABLE INTERRUPT CONTROLLER
// https://wiki.osdev.org/8259_PIC

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xa0
#define PIC2_DATA 0xa1

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01

#define CASCADE_IRQ 0x2

// on master
#define IRQ0 1           // 0000_0001
#define IRQ1 (1 << 1)    // 0000_0010
#define CASCADE (1 << 2) // 0000_0100
// on slave
#define IRQ14 (1 << 6) // 0100_0000

// end of interrupt
void PIC_send_EOI(uint8_t irq) {
  if (irq >= 8)
    outb(PIC2_CMD, PIC_EOI);
  outb(PIC1_CMD, PIC_EOI);
}

void PIC_init() {
  // start pic initialization
  outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
  outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
  // set vector offsets
  outb(PIC1_DATA, PIC_OFFSET);
  outb(PIC2_DATA, PIC_OFFSET + 8);
  // Configure master/slave relationship
  outb(PIC1_DATA, 1 << CASCADE_IRQ);
  outb(PIC2_DATA, CASCADE_IRQ);
  // set 8086 mode
  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);
  // mask every IRQ initially
  outb(PIC1_DATA, 0xff);
  outb(PIC2_DATA, 0xff);
  // unmask IRQ1 (keyboard) on the master pic
  uint8_t master_mask = 0xff;
  uint8_t slave_mask = 0xff;
  master_mask ^= (IRQ0 | IRQ1 | CASCADE);
  slave_mask ^= IRQ14;

  outb(PIC1_DATA, master_mask);
  outb(PIC2_DATA, slave_mask);
}
