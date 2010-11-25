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

#include "midibytepack.hpp"
#include <math.h>

void midiBytePack(std::vector<unsigned char> inputData, std::vector<unsigned char> &outputData){
	unsigned int inBits = inputData.size() * 8;
	//compute the number of bytes needed for the output data
	unsigned int outBytes = inBits / 7 + ((inBits % 7) == 0 ? 0 : 1);
	//resize so that it is easier to convert to c
	outputData.resize(outBytes);
	for(unsigned int i = 0; i < inputData.size(); i++){
		unsigned int pageIndex = 8 * (i / 7);
		switch(i % 7){
			case 0:
				outputData[pageIndex] = inputData[i] >> 1;
				outputData[pageIndex + 1] = (inputData[i] << 6) & 0x7F;
				break;
			case 1:
				outputData[pageIndex + 1] |= inputData[i] >> 2;
				outputData[pageIndex + 2] = (inputData[i] << 5) & 0x7F;
				break;
			case 2:
				outputData[pageIndex + 2] |= inputData[i] >> 3;
				outputData[pageIndex + 3] = (inputData[i] << 4) & 0x7F;
				break;
			case 3:
				outputData[pageIndex + 3] |= inputData[i] >> 4;
				outputData[pageIndex + 4] = (inputData[i] << 3) & 0x7F;
				break;
			case 4:
				outputData[pageIndex + 4] |= inputData[i] >> 5;
				outputData[pageIndex + 5] = (inputData[i] << 2) & 0x7F;
				break;
			case 5:
				outputData[pageIndex + 5] |= inputData[i] >> 6;
				outputData[pageIndex + 6] = (inputData[i] << 1) & 0x7F;
				break;
			case 6:
				outputData[pageIndex + 6] |= inputData[i] >> 7;
				outputData[pageIndex + 7] |= inputData[i] & 0x7F;
				break;
		}
	}
}

