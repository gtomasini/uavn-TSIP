//He desarrollado el proyecto con DEV-C++ sobre el entorno windows simplemente porque el IDE me gusta y no necesitaba
//nada del sistema operativo, aunque luego me di cuenta que tenía que tirar de alguna función de red como ntohl()
//He asumido para las pruebas que el largo del campo data puede ser cero, o sea no estar directamente
//con lo cual el largo minimo del paquete será ID1+ID2+CRC: 6bytes
//básicamente hay dos estados: NO_SYNC, cuando no encontró el primer DLE de comienzo e INCOMPLETE,
//cuando ha sincronizado con el primer DLE pero aún el paquete está incompleto por no haber encontrado el DLE-TXE final
//cuando encuentra el DLE-TXE final automáticamente cambia de estado y regresa a NO_SYNC.
//al tratarse de arquitectura intel (little endian) no tengo que hacer nada con el CRC, pero para
//no depender de esta arquitectura he convertido a big endian ese campo crc y luego la generalizacion
//se logra con ntohl() para lo cual tuve que tirar de algunos headers propios de windows, ya que en este 
//entorno no tengo <arpa/inet.h>
//He desarrollado dos funciones "mockeadas" ProcessValidData() que devuelve un array de prueba "ad hoc"
//y ProcessValidData() que solo muestra el paquete final entregado
//
//Guillermo Tomasini enero 2018

#include <iostream>
#include <cstdint>
#include <unistd.h>
#include "Parser.h"
#include <winsock2.h> 
#include "mylog.h"

int main(int argc, char** argv) {
	Parser p(48);//48 bytes for byffer
	if (p.error)	return -1;
	p.processing();//never rturns
	
	return 0;
}

//for log
// Returns the local date/time formatted as 2014-03-19 11:11:52
char* getFormattedTime(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Must be static, otherwise won't work
    static char _retval[20];
    strftime(_retval, sizeof(_retval), "%Y-%m-%d %H:%M:%S", timeinfo);

    return _retval;
}

//Tests
//mocked function
void ProcessValidData(uint8_t* const buff, const int32_t len){
	static int counter=0;
	LOGBALD("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	LOGBALD("ProcessValidData(%d) call!!!\n", len);
	LOGBALD("Final Packet #%d to process: ", ++counter);
	for(int i=0; i<len; i++) printf("%02x ",buff[i]);
	LOGBALD("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}

//buffer for testing
static const uint8_t testBuff[]{
	//garbage
	0, 0, /*DLEx2 */0x10,0x10, 
	
	//packet #1, offset 4, we can cut in off 9 
	/*DLE START*/5, 0x10, 6, 7, 8, 9, 0x0a, /*DLE x 2*/0x10, 0x10, /*CRC */0x20, 0x75, 0x86, 0xf8, /*DLE-ETX*/0x10, 0x03,
	//                             ^cut here
	
	//packet #2, offset 19, we can cutt in off 20
	/*DLE START*/0x10, 0x14, 0x15, 0x16, 0x17, /*CRC*/ 0x5f, 0x4e, 0x99, 0x7b, /*DLE-ETX */ 0x10, 0x03,
	//                 ^cut here
	
	//packet #3, offset 30 to 38, 
	/*DLE START */0x10, 0x1e, 0x1f, /*CRC*/ 0xd5, 0x20, 0x90, 0x18, /*DLE-ETX*/ 0x10, 0x03,

	//packet #4, offset 39 to 50,  we can cut in off 50
	/*DLE START */0x10, 0x28, 0x29, 0x2a, 0x2b, 0x2c, /*CRC*/0xd0, 0x21, 0xdb, 0x9b, /*DLE ETX*/0x10, 0x03,	
	//                                                                                      ^cut here, off 50
	
	//packet #5, offset 51 to 67
	/*DLE START */ 0x10, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x10, 0x10, 0x3a, /*CRC*/ 0x8f, 0x4d, 0x37, 0xcd, /*DLE-ETX*/ 0x10, 0x03, 
	
	//off 68 -> free space
	0x00, //garbage...
};

//should be modified
static int packSize[]={10, 10, 10, 20, 17, 50, 0, 0 };

//mock
int32_t uavnComRead(uint8_t* const buff, const uint32_t count){
	static int time=0;
	static int off, n;
	static int bal=0;
	
	LOGBALD("--------------------------------\n");
	LOGBALD("uavnComRead(%d): time: %d\n", count, time);
		
	if ( packSize[time]>count){
		int delta=packSize[time]-count;		//delta=100-48=52
		packSize[time]-=delta;				
		packSize[time+1]+=delta;
		for(int i=0; i<7; i++)
			LOGBALD("%d ", packSize[i]);
		LOGBALD("\n");
	}

	memcpy(buff, testBuff+off, packSize[time]);		
	off+=packSize[time];
	
	if (time < 7){
		LOGBALD("returning %d bytes\n",  packSize[time] );
		LOGBALD("--------------------------------\n");
		return packSize[time++];
	}	
	LOGINFO("exit program, bye!!!!!!");
	LOGBALD("--------------------------------\n");
	exit(0);
}


