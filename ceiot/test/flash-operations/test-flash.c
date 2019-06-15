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

#include "contiki.h"
#include "sys/etimer.h"
#include "ext-flash.h"
#include "lib/random.h"

/***********************
 * Structs and constants
 ***********************/

#define HEADER_FLASH_OFFSET 0

#define HEADER_TAG_ID "FDWTV\0"
#define DATA_TAG_ID "TV\0"

typedef struct header_t {
   char tagId[6];
   uint64_t timeInMilis;
   unsigned long diffClock;
} fd_header_t;

typedef struct data_t {
   char tagId[3];
   uint8_t sensorIndex;
   uint32_t value;
   unsigned long clockMark;
   bool sent;
} fd_data_t;

#define DATA_FLASH_OFFSET (HEADER_FLASH_OFFSET + sizeof(fd_header_t))

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

static bool open_flash() {
   bool result = ext_flash_open();
   if (!result) {
      printf("*** ERROR *** Could not open flash memory.\n");
   }
   return result;
}

static bool read_header(fd_header_t *header) {
   bool result = false;
   if (open_flash()) {
      result = ext_flash_read(HEADER_FLASH_OFFSET, sizeof(fd_header_t),
                              (uint8_t *) header);
      if (!result) {
         printf("*** ERROR *** Could not retrived the header from flash memory.\n");
      } else {
         if (strcmp(header->tagId, HEADER_TAG_ID) != 0) {
            result = false;
            printf("*** ERROR *** Header not found in flash memory.\n");
         }
      }
   }
   ext_flash_close();
   return result;
}

static bool read_data(uint16_t index, fd_data_t *data) {
   bool result = false;
   if (open_flash()) {
      result = ext_flash_read(DATA_FLASH_OFFSET + (sizeof(fd_data_t) * index),
                              sizeof(fd_data_t), (uint8_t *) data);
      if (!result) {
         printf("*** ERROR *** Could not retrived data on index %i from flash memory.\n",
                index);
      } else {
         if (strcmp(data->tagId, DATA_TAG_ID) != 0) {
            result = false;
            printf("*** ERROR *** Data not found on index %i from flash memory.\n",
                   index);
         }
      }
   }
   ext_flash_close();
   return result;
}

static bool erase_header() {
   bool result = false;
   if (open_flash()) {
      result = ext_flash_erase(HEADER_FLASH_OFFSET, sizeof(fd_header_t));
      if (!result) {
         printf("*** ERROR *** Could not erase header from flash memory.\n");
      }
   }
   ext_flash_close();
   return result;
}

static bool erase_data(uint16_t index) {
   bool result = false;
   if (open_flash()) {
      result = ext_flash_erase(
      DATA_FLASH_OFFSET + (sizeof(fd_data_t) * index),
                               sizeof(fd_data_t));
      if (!result) {
         printf("*** ERROR *** Could not erase data on index %i from flash memory.\n",
                index);
      }
   }
   ext_flash_close();
   return result;
}

static bool write_header(fd_header_t *header) {
   bool result = false;
   if (open_flash()) {
      result = ext_flash_write(HEADER_FLASH_OFFSET, sizeof(fd_header_t),
                               (uint8_t *) header);
      if (!result) {
         printf("*** ERROR *** Could not write the header on flash memory.\n");
      }
   }
   ext_flash_close();
   return result;
}

static bool write_data(uint16_t index, fd_data_t *data) {
   bool result = false;
   if (open_flash()) {
      result = ext_flash_write(DATA_FLASH_OFFSET + (sizeof(fd_data_t) * index),
                               sizeof(fd_data_t), (uint8_t *) data);
      if (!result) {
         printf("*** ERROR *** Could not write data on index %i on flash memory.\n",
                index);
      }
   }
   ext_flash_close();
   return result;
}

/**************************
 * Processes implementation
 **************************/

PROCESS_THREAD(testFlash, ev, data) {
   PROCESS_BEGIN();

   printf("--------------- FLASH OPERATIONS TEST STARTED ---------------\n");

   static fd_header_t fd_header;
   static fd_data_t fd_data;

   bool result = read_header(&fd_header);

   if (!result) {
      printf("*** Error reading from flash!\n");
      if (!erase_header()) {
         printf("Error erasing flash!\n");
      } else {
         strcpy(fd_header.tagId, HEADER_TAG_ID);
         fd_header.timeInMilis = 1560548223031;
         fd_header.diffClock = clock_seconds();

         if (!write_header(&fd_header)) {
            printf("Error writing on flash!\n");
         } else {
            printf("Data wrote in flash with success!\n");
         }
      }
   } else {
      printf("Flash read with success!\n");
      printf("Data found!\n");
      printf("tagId:       %s\n", fd_header.tagId);
      printf("timeInMilis: %lli\n", fd_header.timeInMilis);
      printf("diffClock:   %lu\n", fd_header.diffClock);

      unsigned int i;
      for (i = 0; i < 10; i++) {
         if (read_data(i, &fd_data)) {
            if (strcmp(fd_data.tagId, DATA_TAG_ID) == 0) {
               printf(">> DATA FOUND! %s, %i, %li, %lu, %i\n", fd_data.tagId,
                      fd_data.sensorIndex, fd_data.value, fd_data.clockMark,
                      fd_data.sent);
            } else {
               printf("<< NO DATA FOUND!\n");
               erase_data(i);
            }
         }
      }

      for (i = 0; i < 10; i++) {
         strcpy(fd_data.tagId, DATA_TAG_ID);
         fd_data.sensorIndex = (random_rand() % 9);
         fd_data.value = (random_rand() % 100000);
         fd_data.clockMark = (random_rand() % 1000);
         fd_data.sent = (random_rand() % 2);
         result = write_data(i, &fd_data);
         printf("## DATA GENERATED! %s, %i, %li, %lu, %i\n", fd_data.tagId,
                fd_data.sensorIndex, fd_data.value, fd_data.clockMark,
                fd_data.sent);
         if (!result) {
            printf("*** ERROR writing DATA to flash!\n");
         }
      }

   }

   ext_flash_close();

   printf("---------------- FLASH OPERATIONS TEST ENDED ----------------\n");

   PROCESS_END();
}
