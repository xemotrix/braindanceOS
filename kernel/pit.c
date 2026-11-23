#include "pit.h"
#include <drivers/ports.h>

// https://wiki.osdev.org/Programmable_Interval_Timer

// PIT input clock freq: 1193182Hz
#define PIT_INPUT_FREQ 1193182

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND 0x43

#define MSK_OP_MODE_RATE_GENERATOR 0x4
#define MSK_AC_MODE_RATE_LOHI 0x30

void PIT_init(uint32_t freq) {
  // like PIT_INPUT_FREQ / freq but rounding to nearest
  uint32_t divisor = (PIT_INPUT_FREQ + freq / 2) / freq;

  outb(PIT_COMMAND, MSK_AC_MODE_RATE_LOHI | MSK_OP_MODE_RATE_GENERATOR);
  outb(PIT_CHANNEL0_DATA, divisor & 0xff);
  outb(PIT_CHANNEL0_DATA, (divisor >> 8) & 0xff);
}
