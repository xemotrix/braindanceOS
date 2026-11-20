#ifndef PIC_H
#define PIC_H
// PROGRAMMABLE INTERRUPT CONTROLLER
#include <stdint.h>

#define PIC_OFFSET 0x20

void PIC_init();
void PIC_send_EOI(uint8_t irq);

#endif
