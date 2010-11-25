/*
 * A sysex bootloader for avr chips.
 * Copyright 2010 Alex Norman
 *
 * This file is part of SysexBoot.
 *
 * SysexBoot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SysexBoot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SysexBoot.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <inttypes.h>
#include <stdbool.h>
#include "bootloadercfg.h"
#include "midibootcommon.h"
#include "bytequeue/bytequeue.h"
#include "sysex_tools.h"

#define SYSEX_BEGIN 0xF0
#define SYSEX_END 0xF7

void (*jump_to_app)(void) = 0x0;

void exit_bootloader(void){
   cli();
   boot_rww_enable();
   GICR = _BV(IVCE);  // enable change of interrupt vectors
   GICR = 0; // move interrupts to application flash section
   jump_to_app();
}

uint8_t midiInBuf[MIDIIN_BUF_SIZE];
volatile byteQueue_t midiByteQueue;

//our midi in interrupt, simply writes to our byte queue
ISR(USART_RXC_vect) {
   uint8_t inByte;
   inByte = UDR;
   bytequeue_enqueue((byteQueue_t *)&midiByteQueue,inByte);
}

void midi_send_byte(uint8_t b) {
   // Wait for empty transmit buffer
   while ( !(UCSRA & _BV(UDRE)) );
   UDR = b;
}

void send_sysex_start(void) {
   uint8_t i = 0;
   midi_send_byte(SYSEX_BEGIN);
   for(i = 0; i < MIDIBOOT_SYSEX_ID_LEN; i++)
      midi_send_byte(midiboot_sysex_id[i]);
}

void send_sysex_end(void) {
   midi_send_byte(SYSEX_END);
}

//The ack is just our sysex id  in a sysex message, that is all
void send_ack(void){
   send_sysex_start();
   send_sysex_end();
}

int main(void) {
   uint8_t i, size, curByte;
   bool inSysexMode = false;
   uint16_t inByteIndex = 0;
   uint16_t pageAddress = 0;

   //page size is 128, need 2 more bytes for page address
   //128 + 2 == 130..
   //we actually don't need so much space but, whatever
   uint8_t tmpUnpackedData[130];
   //once we pack it, it is 149 bytes
   uint8_t tmpPackedData[149];
   midiboot_sysex_t sysexMode = MIDIBOOT_INVALID;

   //set up the bootloader conditions
   BOOTLOADER_INIT

      //exit the bootloader if the bootloader condition isn't met
      if(!BOOTLOADER_CONDITION)
         exit_bootloader();

   INDICATE_BOOTLOADER_MODE

      //move the interrupts into the bootloader portion
      GICR = _BV(IVCE);  // enable change of interrupt vectors
   GICR = _BV(IVSEL); // move interrupts to boot flash section

   //init queue and midi
   bytequeue_init((byteQueue_t *)&midiByteQueue, midiInBuf, MIDIIN_BUF_SIZE);
   //TODO INIT
   //midiInit(MIDI_CLOCK_RATE, true, true);
   sei();

   //do main loop
   while(true){
      //read data from the queue and deal with it
      size = bytequeue_length((byteQueue_t *)&midiByteQueue);
      //deal with input data
      for(i = 0; i < size; i++){
         curByte = bytequeue_get((byteQueue_t *)&midiByteQueue, i);

         if(curByte == SYSEX_BEGIN){
            inSysexMode = true;
            inByteIndex = 0;
            sysexMode = MIDIBOOT_INVALID;
         } else if(curByte == SYSEX_END){
            if(inSysexMode){
               //see what mode we're in
               switch(sysexMode){
                  case MIDIBOOT_LEAVE_BOOT:
                     exit_bootloader();
                     break;
                  case MIDIBOOT_GETPAGESIZE:
                     //if we've been sent the correct size packet then send back our data
                     if(inByteIndex == (1 + MIDIBOOT_SYSEX_ID_LEN)){

                        //pack the contents of SPM_PAGESIZE into tmpPackedData
                        //SPM_PAGESIZE is 2 bytes wide
                        tmpUnpackedData[0] = (uint8_t)(SPM_PAGESIZE >> 8);
                        tmpUnpackedData[1] = (uint8_t)(SPM_PAGESIZE & 0xFF);

                        sysex_bit_pack(tmpPackedData, tmpUnpackedData, 2);

                        send_sysex_start();
                        midi_send_byte(MIDIBOOT_GETPAGESIZE);
                        midi_send_byte(tmpPackedData[0]);
                        midi_send_byte(tmpPackedData[1]);
                        midi_send_byte(tmpPackedData[2]);
                        send_sysex_end();
                     } 
                     break;
                     //actually write the page that has been filled up through MIDIBOOT_FILLPAGE
                  case MIDIBOOT_WRITEPAGE:

                     cli();

                     //erase the page
                     eeprom_busy_wait ();
                     boot_page_erase (pageAddress);
                     boot_spm_busy_wait ();
                     //write the page
                     boot_page_write (pageAddress);     // Store buffer in flash page.
                     boot_spm_busy_wait();       // Wait until the memory is written.

                     sei();

                     send_ack();
                     break;
                     //write data into the temporary page buffer [boot_page_fill]
                  case MIDIBOOT_FILLPAGE:
                     if(inByteIndex > (MIDIBOOT_SYSEX_ID_LEN + 1)) {
                        const uint16_t packedSize = inByteIndex - MIDIBOOT_SYSEX_ID_LEN - 1;
                        const uint16_t unpackedSize = sysex_bit_unpacked_length(packedSize);
                        if(unpackedSize <= 64){
                           //the first two bytes are the address
                           uint16_t bytesToWrite = unpackedSize - 2;
                           uint16_t writeStartAddr;

                           //unpack our addr+data
                           sysex_bit_unpack(tmpUnpackedData, tmpPackedData, packedSize);

                           //grab the start address
                           writeStartAddr = (tmpUnpackedData[0] << 8) | tmpUnpackedData[1];
                           //page address, just zero out the lower bits, used for WRITEPAGE
                           pageAddress = writeStartAddr & ~(SPM_PAGESIZE - 1);

                           cli();

                           //fill the temp page buffer
                           uint8_t * pageData = tmpUnpackedData + 2;
                           uint16_t j;
                           for(j = 0; j < bytesToWrite; j+=2){
                              uint16_t w = (((uint16_t)pageData[j + 1]) << 8) + pageData[j];
                              boot_page_fill (writeStartAddr + j, w);
                           }

                           sei();
                           //ack that we've done this so that we can get some more data
                           send_ack();
                        } 
                     }
                     break;
                  default:
                     break;
               }
            }
            inSysexMode = false;
         } else if(inSysexMode){
            //make sure the sysex prefix matches ours
            if((inByteIndex < MIDIBOOT_SYSEX_ID_LEN) && (curByte != midiboot_sysex_id[inByteIndex])){
               inSysexMode = false;
            } else {
               //we're in sysex mode and we've matched the midiboot_sysex_id
               //the next byte tells us what to do
               if(inByteIndex == MIDIBOOT_SYSEX_ID_LEN){
                  if(curByte == MIDIBOOT_LEAVE_BOOT || 
                        curByte == MIDIBOOT_GETPAGESIZE || 
                        curByte == MIDIBOOT_FILLPAGE ||
                        curByte == MIDIBOOT_WRITEPAGE)
                     sysexMode = curByte;
                  else
                     inSysexMode = false;
                  //if we're filling a page, write it to the tmpPackedData, we'll unpack later
               } else if (sysexMode == MIDIBOOT_FILLPAGE){
                  //the first MIDIBOOT_SYSEX_ID_LEN + 1 bytes have already been dealt with
                  uint16_t index = inByteIndex - MIDIBOOT_SYSEX_ID_LEN - 1;
                  if(index < 43) {
                     tmpPackedData[index] = curByte;
                  } else {
                     //XXX ERROR!!!
                     sysexMode = MIDIBOOT_INVALID;
                     inSysexMode = false;
                  }
               }
               //increment the index
               inByteIndex++;
            }
         }
      }

      //advance the pointer
      bytequeue_remove((byteQueue_t *)&midiByteQueue, size);
   }

   return 0;
}
