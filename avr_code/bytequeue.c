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

#include "bytequeue.h"
#include <avr/interrupt.h>

void byteQueueInit(byteQueue_t * queue, uint8_t * dataArray, byteQueueIndex_t arrayLen){
	queue->length = arrayLen;
	queue->data = dataArray;
	queue->start = queue->end = 0;
}

bool byteQueueEnqueue(byteQueue_t * queue, uint8_t item){
	cli();
	//full
	if(((queue->end + 1) % queue->length) == queue->start){
		sei();
		return false;
	} else {
		queue->data[queue->end] = item;
		queue->end = (queue->end + 1) % queue->length;
		sei();
		return true;
	}
}

byteQueueIndex_t byteQueueLength(byteQueue_t * queue){
	byteQueueIndex_t len;
	cli();
	if(queue->end >= queue->start)
		len = queue->end - queue->start;
	else
		len = (queue->length - queue->start) + queue->end;
	sei();
	return len;
}

//we don't need to avoid interrupts if there is only one reader
uint8_t byteQueueGet(byteQueue_t * queue, byteQueueIndex_t index){
	return queue->data[(queue->start + index) % queue->length];
}

//we just update the start index to remove elements
void byteQueueRemove(byteQueue_t * queue, byteQueueIndex_t numToRemove){
	cli();
	queue->start = (queue->start + numToRemove) % queue->length;
	sei();
}

