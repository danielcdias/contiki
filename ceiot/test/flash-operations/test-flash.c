/**
 * File:   test-flash.c
 * Author: Daniel Carvalho Dias (daniel.dias@gmail.com)
 * Date:   13/06/2019
 *
 * Tests flash memory operations
 *
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "ext-flash.h"
#include "lib/random.h"
#include "lib/crc16.h"
#include "lib/memb.h"

/***********************
 * Structs and constants
 ***********************/

#define FLASH_OPERATION_SUCCESSFUL 0
#define FLASH_OPERATION_SUCCESSFUL_BACKUP 1
#define FLASH_OPERATION_ERROR_OPEN 2
#define FLASH_OPERATION_ERROR_WRITING 3
#define FLASH_OPERATION_ERROR_READING 4
#define FLASH_OPERATION_ERROR_CRC 5
#define FLASH_OPERATION_ERROR_ERASING 6
#define FLASH_OPERATION_NO_DATA 7
#define FLASH_OPERATION_INVALID_BLOCK_POS 8
#define FLASH_OPERATION_INVALID_SECTOR_POS 9

#define BLOCK_SIZE 4096 // 4 KBytes
#define SECTOR_SIZE 256 // Bytes

#define MAIN_HEADER_POSITION 0

#define MAIN_HEADER_PRIMARY_ADDR ((BLOCK_SIZE * MAIN_HEADER_POSITION) + 0)
#define MAIN_HEADER_BACKUP_ADDR ((BLOCK_SIZE * MAIN_HEADER_POSITION) + 256)

#define DATA_BLOCKS_START_POSITION 1
#define DATA_BLOCKS_END_POSITION 255

#define DATA_SECTORS_START_POSITION 1
#define DATA_SECTORS_END_POSITION 16

#define STATUS_TYPE_BOARD 0
#define STATUS_TYPE_SENSORS 1

#define MAIN_HEADER_MAP_BLOCKS_PAGES 8

#define GOOD 0
#define BAD 1

#define TAG_MAIN_HEADER "TMH\0"
#define TAG_BLOCK_HEADER "TSH\0"
#define TAG_STATUS_DATA "TSD\0"

#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)

typedef uint32_t bitmap_word_t;
enum { BITS_PER_WORD = sizeof(bitmap_word_t) * CHAR_BIT };

typedef struct _ct_main_header_t {
   char tag[4];
   uint8_t init_data_block;
   uint8_t next_free_block;
   bitmap_word_t block_map[MAIN_HEADER_MAP_BLOCKS_PAGES];
} ct_main_header_t;
MEMB(alloc_ct_main_header, ct_main_header_t, 2);
typedef struct _fd_main_header_t {
   ct_main_header_t content;
   unsigned short crc;
} fd_main_header_t;
MEMB(alloc_fd_main_header, fd_main_header_t, 2);

typedef struct _ct_block_header_t {
   char tag[4];
   uint8_t init_data_sector;
   uint8_t next_free_sector;
   bitmap_word_t sector_map;
} ct_block_header_t;
MEMB(alloc_ct_block_header, ct_block_header_t, 2);
typedef struct _fd_block_header_t {
   ct_block_header_t content;
   unsigned short crc;
} fd_block_header_t;
MEMB(alloc_fd_block_header, fd_block_header_t, 2);

typedef struct _ct_data_t {
   char tag[4];
   uint8_t status_type;
   uint8_t topic_index;
   char status_data[64];
} ct_data_t;
MEMB(alloc_ct_data, ct_data_t, 2);
typedef struct _fd_data_t {
   ct_data_t content;
   unsigned short crc;
} fd_data_t;
MEMB(alloc_fd_data, fd_data_t, 2);

/******************
 * Global variables
 ******************/

/**********************
 * Processes definition
 **********************/

PROCESS(testFlash, "test flash operations");
AUTOSTART_PROCESSES(&testFlash);

/***********
 * Functions
 ***********/

static void set_bit_on_map(bitmap_word_t *bitmap, int n) {
   bitmap[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}

static void clear_bit_on_map(bitmap_word_t *bitmap, int n) {
   bitmap[WORD_OFFSET(n)] &= ~(1 << BIT_OFFSET(n));
}

static int get_bit_on_map(bitmap_word_t *bitmap, int n) {
   bitmap_word_t bit = bitmap[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
   return bit != 0;
}

static ct_main_header_t* malloc_ct_main_header_t() {
   return memb_alloc(&alloc_ct_main_header);
}

static fd_main_header_t* malloc_fd_main_header_t() {
   return memb_alloc(&alloc_fd_main_header);
}

static ct_block_header_t* malloc_ct_block_reader_t() {
   return memb_alloc(&alloc_ct_block_header);
}

static fd_block_header_t* malloc_fd_block_reader_t() {
   return memb_alloc(&alloc_fd_block_header);
}

static ct_data_t* malloc_ct_data_t() {
   return memb_alloc(&alloc_ct_data);
}

static fd_data_t* malloc_fd_data_t() {
   return memb_alloc(&alloc_fd_data);
}

static void free_ct_main_header_t(ct_main_header_t *p) {
   memb_free(&alloc_ct_main_header, p);
}

static void free_fd_main_header_t(fd_main_header_t *p) {
   memb_free(&alloc_fd_main_header, p);
}

static void free_ct_block_reader_t(ct_block_header_t *p) {
   memb_free(&alloc_ct_block_header, p);
}

static void free_fd_block_reader_t(fd_block_header_t *p) {
   memb_free(&alloc_fd_block_header, p);
}

static void free_ct_data_t(ct_data_t *p) {
   memb_free(&alloc_ct_data, p);
}

static void free_fd_data_t(fd_data_t *p) {
   memb_free(&alloc_fd_data, p);
}

static uint32_t get_block_address(uint8_t position) {
   return position * BLOCK_SIZE;
}

static uint32_t get_sector_address(uint8_t position, uint8_t sector) {
   return (position * BLOCK_SIZE) + (sector * SECTOR_SIZE);
}

static uint8_t flash_open() {
   return ext_flash_open() ? FLASH_OPERATION_SUCCESSFUL : FLASH_OPERATION_ERROR_OPEN;
}

static uint8_t flash_close() {
   ext_flash_close();
   return FLASH_OPERATION_SUCCESSFUL;
}

static uint8_t read_main_header(ct_main_header_t *header) {
   static uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   fd_main_header_t *header_container = malloc_fd_main_header_t();
   if (ext_flash_read(MAIN_HEADER_PRIMARY_ADDR, sizeof(fd_main_header_t),
                      (uint8_t *) header_container)) {
      memcpy(header, &(header_container->content), sizeof(ct_main_header_t));
      if (strcmp(header->tag, TAG_MAIN_HEADER) == 0) {
         unsigned short crc = crc16_data((unsigned char*) header,
                                         sizeof(ct_main_header_t), 0);
         if (crc != header_container->crc) {
            result = FLASH_OPERATION_ERROR_CRC;
         }
      } else {
         result = FLASH_OPERATION_NO_DATA;
      }
   } else {
      result = FLASH_OPERATION_ERROR_READING;
   }
   if (result != FLASH_OPERATION_SUCCESSFUL) {
      result = FLASH_OPERATION_SUCCESSFUL_BACKUP;
      if (ext_flash_read(MAIN_HEADER_BACKUP_ADDR, sizeof(fd_main_header_t),
                         (uint8_t *) header_container)) {
         memcpy(header, &(header_container->content), sizeof(ct_main_header_t));
         if (strcmp(header->tag, TAG_MAIN_HEADER) == 0) {
            unsigned short crc = crc16_data((unsigned char*) header,
                                            sizeof(ct_main_header_t), 0);
            if (crc != header_container->crc) {
               result = FLASH_OPERATION_ERROR_CRC;
            }
         } else {
            result = FLASH_OPERATION_NO_DATA;
         }
      } else {
         result = FLASH_OPERATION_ERROR_READING;
      }
   }
   free_fd_main_header_t(header_container);
   return result;
}

static uint8_t read_block_header(uint8_t position, ct_block_header_t *header) {
   if ((position < DATA_BLOCKS_START_POSITION) || (position > DATA_BLOCKS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_BLOCK_POS;
   }
   uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   fd_block_header_t *header_container = malloc_fd_block_reader_t();
   if (ext_flash_read(get_block_address(position),
                      sizeof(fd_block_header_t),
                      (uint8_t *) header_container)) {
      memcpy(header, &(header_container->content), sizeof(ct_block_header_t));
      if (strcmp(header->tag, TAG_BLOCK_HEADER) == 0) {
         unsigned short crc = crc16_data((unsigned char*) header,
                                         sizeof(ct_block_header_t), 0);
         if (crc != header_container->crc) {
            result = FLASH_OPERATION_ERROR_CRC;
         }
      } else {
         result = FLASH_OPERATION_NO_DATA;
      }
   } else {
      result = FLASH_OPERATION_ERROR_READING;
   }
   free_fd_block_reader_t(header_container);
   return result;
}

static uint8_t read_data(uint8_t position, uint8_t sector, ct_data_t *data) {
   if ((position < DATA_BLOCKS_START_POSITION) || (position > DATA_BLOCKS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_BLOCK_POS;
   }
   if ((sector < DATA_SECTORS_START_POSITION) || (sector > DATA_SECTORS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_SECTOR_POS;
   }
   uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   fd_data_t *data_container = malloc_fd_data_t();
   if (ext_flash_read(get_sector_address(position, sector),
                      sizeof(fd_data_t), (uint8_t *) data_container)) {
      memcpy(data, &(data_container->content), sizeof(ct_data_t));
      if (strcmp(data->tag, TAG_STATUS_DATA) == 0) {
         unsigned short crc = crc16_data((unsigned char*) data,
                                         sizeof(ct_data_t), 0);
         if (crc != data_container->crc) {
            result = FLASH_OPERATION_ERROR_CRC;
         }
      } else {
         result = FLASH_OPERATION_NO_DATA;
      }
   } else {
      result = FLASH_OPERATION_ERROR_READING;
   }
   free_fd_data_t(data_container);
   return result;
}

static uint8_t erase_block(uint8_t position) {
   if ((position < MAIN_HEADER_POSITION) || (position > DATA_BLOCKS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_BLOCK_POS;
   }
   uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   if (!ext_flash_erase(get_block_address(position), BLOCK_SIZE)) {
      result = FLASH_OPERATION_ERROR_ERASING;
   }
   return result;
}

static uint8_t write_main_header(ct_main_header_t *header) {
   uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   fd_main_header_t *header_container = malloc_fd_main_header_t();
   strcpy(header->tag, TAG_MAIN_HEADER);
   memcpy(&(header_container->content), header, sizeof(ct_main_header_t));
   header_container->crc = crc16_data((unsigned char*) header,
                                      sizeof(ct_main_header_t), 0);
   if (ext_flash_write(MAIN_HEADER_PRIMARY_ADDR, sizeof(fd_main_header_t),
                       (uint8_t *) header_container)) {
      if (!ext_flash_write(MAIN_HEADER_BACKUP_ADDR, sizeof(fd_main_header_t),
                           (uint8_t *) header_container)) {
         result = FLASH_OPERATION_ERROR_WRITING;
      }
   } else {
      result = FLASH_OPERATION_ERROR_WRITING;
   }
   free_fd_main_header_t(header_container);
   return result;
}

static uint8_t write_block_header(uint16_t position,
         ct_block_header_t *header) {
   if ((position < DATA_BLOCKS_START_POSITION) || (position > DATA_BLOCKS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_BLOCK_POS;
   }
   uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   fd_block_header_t *header_container = malloc_fd_block_reader_t();
   strcpy(header->tag, TAG_BLOCK_HEADER);
   memcpy(&(header_container->content), header, sizeof(ct_block_header_t));
   header_container->crc = crc16_data((unsigned char*) header,
                                      sizeof(ct_block_header_t), 0);
   if (!ext_flash_write(get_block_address(position),
                        sizeof(fd_block_header_t),
                        (uint8_t *) header_container)) {
      result = FLASH_OPERATION_ERROR_WRITING;
   }
   free_fd_block_reader_t(header_container);
   return result;
}

static uint8_t write_data(uint16_t position, uint16_t sector, ct_data_t *data) {
   if ((position < DATA_BLOCKS_START_POSITION) || (position > DATA_BLOCKS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_BLOCK_POS;
   }
   if ((sector < DATA_SECTORS_START_POSITION) || (sector > DATA_SECTORS_END_POSITION)) {
      return FLASH_OPERATION_INVALID_SECTOR_POS;
   }
   uint8_t result = FLASH_OPERATION_SUCCESSFUL;
   fd_data_t *data_container = malloc_fd_data_t();
   strcpy(data->tag, TAG_STATUS_DATA);
   memcpy(&(data_container->content), data, sizeof(ct_data_t));
   data_container->crc = crc16_data((unsigned char*) data, sizeof(ct_data_t),
                                    0);
   if (!ext_flash_write(get_sector_address(position, sector),
                        sizeof(fd_data_t), (uint8_t *) data_container)) {
      result = FLASH_OPERATION_ERROR_WRITING;
   }
   free_fd_data_t(data_container);
   return result;
}

/**************************
 * Processes implementation
 **************************/

PROCESS_THREAD(testFlash, ev, data) {
   PROCESS_BEGIN();

   printf("------------------------ FLASH OPERATIONS TEST STARTED! ------------------------\n");

   printf("Sizeof main header : %u\n", sizeof(fd_main_header_t));
   printf("Sizeof block header: %u\n", sizeof(fd_block_header_t));
   printf("Sizeof data        : %u\n", sizeof(fd_data_t));

   ct_main_header_t *main_header_writer = malloc_ct_main_header_t();
   ct_main_header_t *main_header_reader = malloc_ct_main_header_t();
   ct_block_header_t *block_header_writer = malloc_ct_block_reader_t();
   ct_block_header_t *block_header_reader = malloc_ct_block_reader_t();
   ct_data_t *data_writer = malloc_ct_data_t();
   ct_data_t *data_reader = malloc_ct_data_t();

   if (flash_open() == FLASH_OPERATION_SUCCESSFUL) {

      switch (erase_block(MAIN_HEADER_PRIMARY_ADDR)) {
         case FLASH_OPERATION_SUCCESSFUL:
            main_header_writer->init_data_block = 1;
            main_header_writer->next_free_block = 1;
            main_header_writer->block_map[0] = 101;
            main_header_writer->block_map[1] = 202;
            main_header_writer->block_map[2] = 303;
            main_header_writer->block_map[3] = 404;
            main_header_writer->block_map[4] = 505;
            main_header_writer->block_map[5] = 606;
            main_header_writer->block_map[6] = 707;
            main_header_writer->block_map[7] = 808;
            switch (write_main_header(main_header_writer)) {
               case FLASH_OPERATION_SUCCESSFUL_BACKUP:
                  printf("### WARNING ### Using backup header data.\n");
               case FLASH_OPERATION_SUCCESSFUL:
                  switch (read_main_header(main_header_reader)) {
                     case FLASH_OPERATION_SUCCESSFUL:
                        printf("main_header->tag            : %s\n",
                               main_header_reader->tag);
                        printf("main_header->init_data_block: %u\n",
                               main_header_reader->init_data_block);
                        printf("main_header->next_free_block: %u\n",
                               main_header_reader->next_free_block);
                        static uint8_t i;
                        for (i = 0; i < MAIN_HEADER_MAP_BLOCKS_PAGES; i++) {
                           printf("header->block_map      : %lu\n",
                                  main_header_reader->block_map[i]);
                        }
                        break;
                     case FLASH_OPERATION_ERROR_READING:
                        printf("*** ERROR *** Could not read from main header position in flash.\n");
                        break;
                     case FLASH_OPERATION_ERROR_CRC:
                        printf("*** ERROR *** CRC not match for data read from main header position in flash.\n");
                        break;
                     case FLASH_OPERATION_NO_DATA:
                        printf("*** ERROR *** No data found for main header position in flash.\n");
                        break;
                     default:
                        printf("*** ERROR *** Not expected error when reading main header.\n");
                        break;
                  }
                  break;
               case FLASH_OPERATION_ERROR_WRITING:
                  printf("*** ERROR *** Could not write in header position in flash.\n");
                  break;
               default:
                  printf("*** ERROR *** Not expected error when writing main header.\n");
                  break;
            }
            break;
         case FLASH_OPERATION_ERROR_ERASING:
            printf("*** ERROR *** Could not erase the header position in flash.\n");
            break;
         default:
            printf("*** ERROR *** Not expected error when erasing main header.\n");
            break;
      }

      switch (erase_block(1)) {
         case FLASH_OPERATION_SUCCESSFUL:
            block_header_writer->init_data_sector = 12;
            block_header_writer->next_free_sector = 12;
            block_header_writer->sector_map = 11011;
            switch (write_block_header(1, block_header_writer)) {
               case FLASH_OPERATION_SUCCESSFUL:
                  switch (read_block_header(1, block_header_reader)) {
                     case FLASH_OPERATION_SUCCESSFUL:
                        printf("block_header->tag            : %s\n",
                               block_header_reader->tag);
                        printf("block_header->init_data_block: %u\n",
                               block_header_reader->init_data_sector);
                        printf("block_header->next_free_block: %u\n",
                               block_header_reader->next_free_sector);
                        printf("block_header->block_map      : %lu\n",
                               block_header_reader->sector_map);
                        break;
                     case FLASH_OPERATION_ERROR_READING:
                        printf("*** ERROR *** Could not read from block header position %u in flash.\n", 1);
                        break;
                     case FLASH_OPERATION_ERROR_CRC:
                        printf("*** ERROR *** CRC not match for data read from block header position %u in flash.\n", 1);
                        break;
                     case FLASH_OPERATION_NO_DATA:
                        printf("*** ERROR *** No data found for block header position %u in flash.\n", 1);
                        break;
                     default:
                        printf("*** ERROR *** Not expected error when reading block header.\n");
                        break;
                  }
                  break;
               case FLASH_OPERATION_ERROR_WRITING:
                  printf("*** ERROR *** Could not write in block header position %u in flash.\n", 1);
                  break;
               default:
                  printf("*** ERROR *** Not expected error when writing block header.\n");
                  break;
            }

            data_writer->status_type = STATUS_TYPE_SENSORS;
            data_writer->topic_index = 7;
            strcpy(data_writer->status_data, "X-------------------------------------------------------------X\0");
            switch (write_data(1, 1, data_writer)) {
               case FLASH_OPERATION_SUCCESSFUL:
                  switch (read_data(1, 1, data_reader)) {
                     case FLASH_OPERATION_SUCCESSFUL:
                        printf("data->tag        : %s\n",
                               data_reader->tag);
                        printf("data->status_type: %u\n",
                               data_reader->status_type);
                        printf("data->topic_index: %u\n",
                               data_reader->topic_index);
                        printf("data->status_data: %s\n",
                               data_reader->status_data);
                        break;
                     case FLASH_OPERATION_ERROR_READING:
                        printf("*** ERROR *** Could not read data from position %u, sector %u in flash.\n", 1, 1);
                        break;
                     case FLASH_OPERATION_ERROR_CRC:
                        printf("*** ERROR *** CRC not match for data read from data position %u, sector %u in flash.\n", 1, 1);
                        break;
                     case FLASH_OPERATION_NO_DATA:
                        printf("*** ERROR *** No data found for data position %u, sector %u in flash.\n", 1, 1);
                        break;
                     default:
                        printf("*** ERROR *** Not expected error when reading data.\n");
                        break;
                  }
                  break;
               case FLASH_OPERATION_ERROR_WRITING:
                  printf("*** ERROR *** Could not write data in position %u, sector %u in flash.",1 , 1);
                  break;
               default:
                  printf("*** ERROR *** Not expected error when writing data.\n");
                  break;
            }

            break;
         case FLASH_OPERATION_ERROR_ERASING:
            printf("*** ERROR *** Could not erase the header position in flash.");
            break;
         default:
            printf("*** ERROR *** Not expected error when erasing block header.\n");
            break;
      }
   } else {
      printf("*** ERROR *** Could not open flash drive.");
   }
   flash_close();

   free_ct_main_header_t(main_header_reader);
   free_ct_main_header_t(main_header_writer);
   free_ct_block_reader_t(block_header_reader);
   free_ct_block_reader_t(block_header_writer);
   free_ct_data_t(data_reader);
   free_ct_data_t(data_writer);

   printf("-------------------------- FLASH OPERATIONS TEST ENDED -------------------------\n");

   PROCESS_END();
}
