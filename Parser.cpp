#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "Parser.h"
#include "mylog.h"

Parser::Parser(int32_t n):N(n),freeOff(0),packOff(0),
	state(NO_SYNC),error(false){

	buff=(uint8_t*)malloc(n*sizeof(uint8_t));
	if (!buff){
		LOGERROR("Error allocating memory for Buffer!");
		error=true;
	}
	//finalBuffer size is 1/4 of buff
	//this buffer only grows if necessary
	finalBuff=(uint8_t*)malloc(n*sizeof(uint8_t)/4);
	if (!finalBuff){
		LOGERROR("Error allocating memory for finalBuffer!");
		error=true;
	}
	finalBuffLen=n/4;
}

//duplicate buff size
int Parser::resize(){
	N*=2;
	buff = (uint8_t*) realloc(buff, N*sizeof(uint8_t));
	if (buff==NULL){
		LOGERROR("resize): ERROR (re)allocating memory!");
		N/=2;
		error=true;
		return -1;
	}
	LOGINFO("resize): Buffer resized to N=%d!!\n", N);
	error=false;
	return 0;
}

//shift to 0 pos incomplete pack in order to get free buffer		
int Parser::shiftIncompletePack(){
	if (freeOff==0 || packOff==0 ) return -1;
	
	if ( packOff < freeOff - packOff){
		LOGDEBUG("shiftIncompletePack: for() packOff=%d freeOff=%d\n", packOff, freeOff);
		//I don't use memcpy() in order of not losing bytes, because it overlaps
		for(int i=0; i< freeOff-packOff; i++)
			buff[i]=buff[packOff+i];
	}
	else{//move last incomplete buffer to 0 off, step by step
		LOGDEBUG("tshiftIncompletePack: memcpy() packOff=%d freeOff=%d\n", packOff, freeOff);
		memcpy(buff, buff+packOff, freeOff-packOff);
	}	
	//adjust offsets
	freeOff	-=packOff;
	parseOff-=packOff;
	packOff=0;
	LOGDEBUG("Tiny buff: packOff=%d freeOff=%d\n", packOff, freeOff);
	return 0;
}

void Parser::processing(){

	parseOff = freeOff;
	uint32_t outOfBuff=0;//counter
	for(uint32_t k=1;;k++){
		LOGDEBUG("Processing ===> freeOff=%d packOff=%d N=%d state=%d free=%d\%\n", 
			freeOff, packOff, N, state, (N-freeOff)*100/N );

		evaluateAskMemory(k, outOfBuff);
		int32_t n=uavnComRead(buff+freeOff, N-freeOff);
		if (n<=0) continue;//no data
	
		bool readMore=false;
		int i;
		do{
			switch(state){
				//change to INCOMPLETE when start DLE is found
				case NO_SYNC:
					LOGDEBUG("STATE: NO SYNC, %d read bytes\n", n);
					for (i=parseOff; i < freeOff+n; i++){
						LOGBALD("[%d]=%02x ",i, buff[i]);
						if(buff[i] != DLE) continue;
						//DLE match
						if (i+1 == n+freeOff){//out of read bytes!
							LOGBALD("DLE match, but out of read bytes..., set freeOff=1!\n");
							readMore=true;
							buff[0]	=DLE;
							freeOff	=1;
							parseOff=0;
							break;
						} 
						if(buff[i+1]==ETX||buff[i+1]==DLE){
							LOGBALD("[%d]=%02x ",i, buff[i+1]);
							LOGBALD("(DLE/DLE-ETX) ");
							i++;//skip next byte (DLE or ETX)
							continue;
						}
						//starting DLE found!
						state	=INCOMPLETE;//Change
						packOff	=i;
						parseOff=i+1;
						LOGDEBUG(" START DLE match!\n", n, freeOff);
						break;
					}
					LOGBALD("\n");
					if (state==NO_SYNC && !readMore){
						LOGDEBUG("Free buffer!\n");
						freeOff=parseOff=0;
						readMore=true;
					}
					break;
					
				//change to NO SYNC when start DLE-ETX is found
				case INCOMPLETE:
					LOGDEBUG("STATE: INCOMPLETE, %d read bytes\n", n);
					for (i=parseOff; i < n+freeOff; i++){
						LOGBALD("[%d]=%02x ",i, buff[i]);
						if(buff[i] != DLE) continue;
						if(i+1 == n+freeOff){
							LOGDEBUG("DLE match, but out of read bytes...\n");
							readMore=true;
							freeOff=i+1;
							parseOff=i;
							break;
						}
						if(buff[i+1]==DLE){
							LOGBALD("[%d]=%02x ",i+1, buff[i]);
							LOGBALD("(DLEx2 match) ");
							i++;//skip next byte(DLE)
							continue;
						}
						if(buff[i+1]==ETX){
							LOGBALD("[%d]=%02x ",i+1, buff[i+1]);
							LOGBALD("FULL packet match (%i, %i)!!\n", packOff, i);
							deliverPacket(packOff+1, i);
							parseOff=i+2;//skip 2 final bytes (DLE-ETX)
							state=NO_SYNC;
							break;
						}
						//just one DLE, so reset packOff!!!
						LOGWARN("sync: just one DLE found, would be a start!\n");
						packOff=i;//starts again
					}
					if (state==INCOMPLETE && !readMore){
						readMore=true;
						//TODO!!! change this!!!
						if (i >= N)	resize();//not enough free buffer
						freeOff	=i;
						parseOff=i;
					}
					break;
					
				default:
					LOGERROR("STATE ERROR!!!!");
					exit(-1);
			}//switch
		} while(!readMore);
	}//for(;;)
}

int Parser::deliverPacket(uint32_t off0, uint32_t offF){
	LOGDEBUG(" off0: %d, offF: %d\n", off0, offF);
	uint32_t len=offF-off0;
	if ( len < MIN_PACK_LEN) return -1;//so small
	
	if (finalBuffLen < len){//I need more buffer space
		LOGDEBUG("resize is needed for finalBuff!\n");
		finalBuff = (uint8_t*) realloc(finalBuff, len*sizeof(uint8_t));
		if (finalBuff==NULL){
			LOGDEBUG("deliverPacket(): ERROR (re)allocating memory!\n");
			return -1;
		}
		finalBuffLen=len;//new len
	}
		
	//copy from buff to finalBuff and clean LDE's 
	for(uint32_t i=0, j=0, k; i<len; i++, j++){
		//Just ask if there are two LDEs followed
		if (buff[off0+j]==DLE && j+1<len && buff[off0+j+1]==DLE){
			++j;
			--len;
		}
		finalBuff[i]=buff[off0+j];
	}
	
	if ( len < MIN_PACK_LEN) return -1;//packet so small

	LOGBALD("===> finalBuf(%d): ", len);
	for(int i=0; i<len; i++) LOGBALD("%02x ",finalBuff[i]);
	LOGBALD("<=====\n");
	
	uint32_t *receivedCRC=(uint32_t*)&finalBuff[len-4];
	uint32_t calculatedCRC=crc32(finalBuff,  0, len-4);
	
	LOGDEBUG("Received CRC vs Calculated from packet: %u, %u\n", littleIndian2host(*receivedCRC),calculatedCRC);
	
	if (calculatedCRC == littleIndian2host(*receivedCRC)){
		ProcessValidData(finalBuff, len-4);
	}
	else{
		LOGDEBUG("CRC NO OK, drop packet!\n");
		return -1;
	}
		
	return 0;	
}

//ask for memory if less than 10% of buffer is left free
int Parser::evaluateAskMemory(uint32_t &times, uint32_t &outOfBufCounter){
	if (!times) times=1;//times shouldn't be zero!!!
	if ( (N-freeOff)*100/N < 10){//less than 10% of free space, not enough buffer
		LOGWARN("NOT enough remaining free buffer, freq=%i\%!\n", 
					(outOfBufCounter+1)*100/times);

		if (++outOfBufCounter*100/times > 50){//>50% of times out of buffer so it is better to resize
			LOGWARN(">50% of times out of buffer!!" );
			times=outOfBufCounter=0;
			return resize();
		}
		//free buffer
		shiftIncompletePack();
	}
	return 0;
}

