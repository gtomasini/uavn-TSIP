#ifndef PARSER_H
#define PARSER_H

#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h> 

/* States:
NO_SINC: no sincronization, no starting DLE found, offset=n
INCOMPLETED: start DLE found but buffer doesn't reach end DLE ETX. offset=n 
*/

uint32_t crc32(uint8_t *buf, uint32_t start, uint32_t end);
void ProcessValidData(uint8_t* const buff, const int32_t numberOfBytes);
int32_t uavnComRead(uint8_t* const buff, const uint32_t count);

class Parser{
	public:

	Parser(uint32_t n);

	const uint8_t DLE=0x10;
	const uint8_t ETX=0x03;
	const uint32_t MIN_PACK_LEN=6;
	bool	error;
	enum State : int { NO_SYNC, INCOMPLETE };
	
	uint8_t *buff;//raw packet buffer used to read
	uint32_t N;//buff len						
	uint8_t *finalBuff;//final payload and processed data buffer
	uint32_t finalBuffLen;
	State state;
	uint32_t packOff;//offset to the begining of the packet (LDE)
	uint32_t freeOff;//offset to the begining of the free zone.
	uint32_t parseOff;//offset to the beginint of the buffer to parse
	
	//reads from uavnComRead(buff+freeOff, N-freeOff);
	//never returns 
	void processing();
	
	//convert littleIndian to host byte order, in this case (Intel) it's not mandatory	
	inline uint32_t littleIndian2host(uint32_t littleIndian){
		uint8_t	aux32[4], bigEndianCRC[4];

		memcpy(aux32, &littleIndian, 4);
		bigEndianCRC[0]=aux32[3];
		bigEndianCRC[1]=aux32[2];
		bigEndianCRC[2]=aux32[1];
		bigEndianCRC[3]=aux32[0];
		uint32_t	x32;
		memcpy(&x32, bigEndianCRC, 4);
		return ntohl(x32);
	}
	
	int deliverPacket(uint32_t off0, uint32_t offF);
	//resize buffer
	int resize();
	int resize(uint8_t newsize);
	
	//shift to 0 pos incomplete pack in order to get free buffer
	int shiftIncompletePack();
	//ask for memory if less than 10% of buffer is left free
	int evaluateAskMemory(uint32_t &times, uint32_t &outOfBufCounter);
	virtual ~Parser(){free(buff);free(finalBuff);};
};

#endif