#ifndef _FIFO_MODULE_
#define _FIFO_MODULE_

#include "limits.h"
#include "stdint.h"

#define BUFFER_SIZE 32


#define EXIT_SUCESS		SCHAR_MIN			 //all OK
#define ENODATA			SCHAR_MIN +1   		 //error no Data in FIFO
#define EIFLEN      	SCHAR_MIN +2         //error invalid fifo LEN
#define ENOBUFS		 	SCHAR_MIN +3 	     //error buffer full (FIFO Full)
#define EPERM			SCHAR_MIN +4	     //operation not permited
#define EICHAR      	SCHAR_MIN +5         //error invalid char


//Struct to handle fifo
typedef struct st_fifo{
	uint8_t* fifo;             //pointer to FIFO buffer
	uint8_t index_write;
	uint8_t index_read;
	uint8_t fifo_len;          //tamanho da fifo
	uint8_t fifo_dead;
}ST_FIFO;



//
// init an empty FIFO
//
int8_t fifo_init(ST_FIFO *stfifo);

//
// kill a FIFO
//
int8_t fifo_kill(ST_FIFO *stfifo);

//
// insert an element into FIFO (push)
//
int8_t fifo_push(ST_FIFO *stfifo,uint8_t c);

//
// removes first element from FIFO
//
int8_t fifo_pop(ST_FIFO *stfifo);

//
// removes last element from FIFO
//
int8_t fifo_pop_last(ST_FIFO *stfifo);

//
// get last completed message from FIFO
//
int8_t fifo_get_message(ST_FIFO *stfifo, char* out);

//
// get number of elements on the FIFO
//
uint8_t fifo_size(ST_FIFO *stfifo);




#endif //final  de _FIFO_MODULE_
