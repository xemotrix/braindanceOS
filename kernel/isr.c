#include "panic.h"
// INTERRUPT SERVICE ROUTINE

// division by 0
void isr0_handler() { PANIC("EXCEPTION: DIVISION BY 0\n"); }

// page fault
void isr14_handler() { PANIC("PAGEFAUlT???\n"); }
