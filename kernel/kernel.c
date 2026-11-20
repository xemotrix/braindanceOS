#include "../drivers/ata.h"
#include "fat32.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "pit.h"
#include "pmm.h"
#include "scheduler.h"
#include "tty.h"
#include "vmm.h"

void startup() {
  ata_init();
  vfs_init_data fsops = fat32_init();
  vfs_init(fsops);
  scheduler_create_kernel_task(&run_tty, "tty");
  kexit();
}

int main() {
  idt_init();
  PIC_init();
  PIT_init(1000);

  STI; // Enable cpu interrupts

  pmm_init();
  vmm_init();

  scheduler_create_kernel_task(&startup, "start");
  scheduler_init();
}
