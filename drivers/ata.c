#include "ata.h"
#include "../kernel/panic.h"
#include "../kernel/scheduler.h"
#include "../kernel/sync.h"
#include "../utils/stdio.h"
#include "ports.h"
#include <stddef.h>

// https://wiki.osdev.org/ATA_PIO_Mode
// https://wiki.osdev.org/ATA_Command_Matrix

#define PORT_DATA 0x1F0         // RW
#define PORT_ERROR 0x1F1        // R
#define PORT_FEATURES 0x1F1     // W
#define PORT_SECTOR_CT 0x1F2    // RW
#define PORT_LBA_LOW 0x1F3      // RW LBAlo
#define PORT_LBA_MID 0x1F4      // RW LBAmid
#define PORT_LBA_HIGH 0x1F5     // RW LBAhi
#define PORT_DRIVE_SELECT 0x1F6 // RW
#define PORT_STATUS 0x1F7       // R
#define PORT_COMMAND 0x1F7      // W

#define PORT_ALT_STATUS 0x3F6  // R
#define PORT_DVC_CONTROL 0x3F6 // W (yes the same)
#define PORT_DRV_ADDRESS 0x3F7 // R

#define IDENTIFY_MASTER_DRIVE_CODE 0xA0
#define IDENTIFY_SLAVE_DRIVE_CODE 0xB0

#define MASTER_DRIVE_CODE 0x40
#define SLAVE_DRIVE_CODE 0x50

#define CMD_READ_SECTORS_EXT 0x24
#define CMD_WRITE_SECTORS_EXT 0x34
#define CMD_CACHE_FLUSH 0xE7
#define CMD_IDENTIFY 0xEC

#define STATUS_MSK_ERR 1
#define STATUS_MSK_IDX 2
#define STATUS_MSK_CORR 4
#define STATUS_MSK_DRQ 8
#define STATUS_MSK_SRV 16
#define STATUS_MSK_DF 32
#define STATUS_MSK_RDY 64
#define STATUS_MSK_BSY 128
#define STATUS_MSK_BSY2 0x80

#define HIGH_BYTE(x) (((x) >> 8) & 0xff)
#define LOW_BYTE(x) ((x) & 0xff)
#define GET_BYTE(num, nbyte) (((num) >> (8 * (nbyte))) & 0xff)

#define SECTOR_SIZE 512
#define WORDS_PER_SECTOR 256

// uint16_t ata_identify_data[256];
struct ata_identify_data_raw {
  uint16_t general_config;
  uint16_t __padding0[9];
  uint16_t serial[10];
  uint16_t __padding1[3];
  uint16_t firmware_rev[4];
  uint16_t model_no[20];
  uint16_t __padding3[53];
  uint64_t addressable_sectors;
  uint16_t __padding4[152];
} __attribute__((packed));

struct ata_identify_data {
  uint16_t general_config;
  char serial[10 * 2 + 1];
  char firmware_rev[4 * 2 + 1];
  char model_no[20 * 2 + 1];
  uint64_t addressable_sectors;
};

struct ata_identify_data_raw ata_identify_data_raw;
struct ata_identify_data ata_identify_data;

mutex_t ata_mtx;
wait_queue_id irq14_wq;

//===================================
void ata_read_disk(uint64_t lba, uint16_t n_sectors, void *buff) {
  sync_mutex_acquire(ata_mtx);

  outb(PORT_DRIVE_SELECT, SLAVE_DRIVE_CODE);
  outb(PORT_SECTOR_CT, HIGH_BYTE(n_sectors));
  outb(PORT_LBA_LOW, GET_BYTE(lba, 3));
  outb(PORT_LBA_MID, GET_BYTE(lba, 4));
  outb(PORT_LBA_HIGH, GET_BYTE(lba, 5));
  outb(PORT_SECTOR_CT, LOW_BYTE(n_sectors));
  outb(PORT_LBA_LOW, GET_BYTE(lba, 0));
  outb(PORT_LBA_MID, GET_BYTE(lba, 1));
  outb(PORT_LBA_HIGH, GET_BYTE(lba, 2));

  SCH_DO_AND_WAIT_FOR(irq14_wq, { outb(PORT_COMMAND, CMD_READ_SECTORS_EXT); });

  for (size_t sector_i = 0; sector_i < n_sectors; sector_i++) {
    uint8_t status;
    SCH_WAIT_WHILE_COND(irq14_wq,
                        !((status = inb(PORT_STATUS)) & STATUS_MSK_DRQ));
    if (status & STATUS_MSK_ERR)
      PANIC("error reading ATA\n");

    for (int i = 0; i < WORDS_PER_SECTOR; i++) {
      ((uint16_t *)buff)[i + WORDS_PER_SECTOR * sector_i] = inw(PORT_DATA);
    }
  }
  uint8_t status = inb(PORT_STATUS);
  if (status & STATUS_MSK_ERR)
    PANIC("error reading ATA\n");

  sync_mutex_release(ata_mtx);
}

void ata_write_disk(uint64_t lba, uint16_t n_sectors, void *buff) {
  sync_mutex_acquire(ata_mtx);

  outb(PORT_DRIVE_SELECT, SLAVE_DRIVE_CODE);
  outb(PORT_SECTOR_CT, HIGH_BYTE(n_sectors));
  outb(PORT_LBA_LOW, GET_BYTE(lba, 3));
  outb(PORT_LBA_MID, GET_BYTE(lba, 4));
  outb(PORT_LBA_HIGH, GET_BYTE(lba, 5));
  outb(PORT_SECTOR_CT, LOW_BYTE(n_sectors));
  outb(PORT_LBA_LOW, GET_BYTE(lba, 0));
  outb(PORT_LBA_MID, GET_BYTE(lba, 1));
  outb(PORT_LBA_HIGH, GET_BYTE(lba, 2));

  outb(PORT_COMMAND, CMD_WRITE_SECTORS_EXT);

  // poll, on read there is no interrupt here
  uint8_t status = inb(PORT_ALT_STATUS);
  while (status & STATUS_MSK_BSY || !(status & STATUS_MSK_DRQ)) {
    if (!(status & STATUS_MSK_BSY) && status & STATUS_MSK_ERR)
      PANIC("error writing ATA\n");
    status = inb(PORT_ALT_STATUS);
  }

  for (size_t sector_i = 0; sector_i < n_sectors; sector_i++) {
    uint8_t status;
    SCH_WAIT_WHILE_COND(irq14_wq,
                        !((status = inb(PORT_STATUS)) & STATUS_MSK_DRQ));
    if (status & STATUS_MSK_ERR)
      PANIC("error writing ATA\n");

    for (int i = 0; i < WORDS_PER_SECTOR; i++) {
      uint16_t word = ((uint16_t *)buff)[i + WORDS_PER_SECTOR * sector_i];
      outw(PORT_DATA, word);
    }
  }

  SCH_WAIT_WHILE_COND(irq14_wq, (status = inb(PORT_STATUS)) & STATUS_MSK_BSY);
  if (inb(PORT_ALT_STATUS) & STATUS_MSK_ERR)
    PANIC("error writing ATA\n");

  outb(PORT_COMMAND, CMD_CACHE_FLUSH);

  SCH_WAIT_WHILE_COND(irq14_wq, (status = inb(PORT_STATUS)) & STATUS_MSK_BSY);

  sync_mutex_release(ata_mtx);
}

static void fix_string(uint16_t *src, char *dst, size_t nwords) {
  for (size_t i = 0; i <= nwords; i++) {
    dst[2 * i] = src[i] >> 8;
    dst[2 * i + 1] = src[i] & 0xff;
  }
  dst[nwords * 2 - 1] = '\0';
}

void ata_init() {
  irq14_wq = scheduler_create_wait_queue();
  ata_mtx = sync_new_mutex();
  outb(PORT_DRIVE_SELECT, IDENTIFY_SLAVE_DRIVE_CODE);
  outb(PORT_LBA_LOW, 0);
  outb(PORT_LBA_MID, 0);
  outb(PORT_LBA_HIGH, 0);

  SCH_DO_AND_WAIT_FOR(irq14_wq, { outb(PORT_COMMAND, CMD_IDENTIFY); });

  uint8_t drive_exists = inb(PORT_COMMAND);
  if (!drive_exists) {
    kprintf("drive does not exist\n");
    return;
  }

  kprintf("drive EXISTS\n");

  while (inb(PORT_STATUS) & STATUS_MSK_BSY) {
    milli_sleep(1);
  }
  uint8_t err = 0;
  while (!(inb(PORT_STATUS) & STATUS_MSK_DRQ) &&
         (err = !(inb(PORT_STATUS) & STATUS_MSK_ERR))) {
    milli_sleep(1);
  }
  if (err)
    PANIC("error! :(");

  for (size_t i = 0; i < 256; i++) {
    ((uint16_t *)&ata_identify_data_raw)[i] = inw(PORT_DATA);
  }
  ata_identify_data.general_config = ata_identify_data_raw.general_config;
  fix_string(ata_identify_data_raw.serial, ata_identify_data.serial, 10);
  fix_string(ata_identify_data_raw.firmware_rev, ata_identify_data.firmware_rev,
             4);
  fix_string(ata_identify_data_raw.model_no, ata_identify_data.model_no, 20);
  ata_identify_data.addressable_sectors =
      ata_identify_data_raw.addressable_sectors;

  kprintf("general_config: %x\n", ata_identify_data.general_config);
  kprintf("serial: %s\n", ata_identify_data.serial);
  kprintf("firmware_rev: %s\n", ata_identify_data.firmware_rev);
  kprintf("model_no: %s\n", ata_identify_data.model_no);
  kprintf("sectors: %x\n", ata_identify_data.addressable_sectors);

  return;
}

void ata_irq_handler() {
  scheduler_wakeup(irq14_wq);
  inb(PORT_STATUS);
}
