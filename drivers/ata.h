#ifndef ATA_H
#define ATA_H
#include <stdint.h>

void ata_read_disk(uint64_t lba, uint16_t n_sectors, void *buff);
void ata_write_disk(uint64_t lba, uint16_t n_sectors, void *buff);
void ata_irq_handler();
void ata_init();

#endif
