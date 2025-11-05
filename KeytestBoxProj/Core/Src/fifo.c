#include "fifo.h"
#include "stdint.h"
#include "string.h"


//init an empty stfifo
//
int8_t fifo_init(ST_FIFO *stfifo)
{

	stfifo->index_write = 0;
	stfifo->index_read = 0;
	stfifo->fifo_dead = 0;
	stfifo->fifo_len = BUFFER_SIZE;

	return EXIT_SUCESS;
}


//kill the fifo
//
int8_t fifo_kill(ST_FIFO *stfifo)
{

	if(stfifo->fifo_dead)
	{
		return EPERM;
	}

	stfifo->fifo_dead=1;	      	//stfifo cannot be used until stfifo_init

	return EXIT_SUCESS;
}

//insert element on our fifo
//
int8_t fifo_push(ST_FIFO *stfifo,uint8_t c)
{

	if(stfifo->fifo_dead)
	{
		return EPERM;
	}

	//is stfifo full?
	if((stfifo->index_write-stfifo->index_read)>=stfifo->fifo_len)
	{
		return ENOBUFS;				//error stfifo full
	}

	stfifo->fifo[stfifo->index_write & (stfifo->fifo_len-1)]=c;
	stfifo->index_write++;

	return EXIT_SUCESS;
}


//remove first element from fifo
//

int8_t fifo_pop(ST_FIFO *stfifo)
{

	uint8_t c;

	if(stfifo->fifo_dead)
	{
		return EPERM;
	}

	if(!(stfifo->index_write^stfifo->index_read))
	{
		return ENODATA;
	}

	c=stfifo->fifo[stfifo->index_read & (stfifo->fifo_len-1)];
	stfifo->index_read++;

	return c;
}

//remove last element from fifo
//
int8_t fifo_pop_last(ST_FIFO *stfifo)
{

	uint8_t c;

	if(stfifo->fifo_dead)
	{
		return EPERM;
	}

	if(!(stfifo->index_write^stfifo->index_read))
	{
		return ENODATA;
	}

	c=stfifo->fifo[stfifo->index_write];
	stfifo->index_write--;

	return c;
}

int8_t fifo_get_message(ST_FIFO *stfifo, char* out)
{
    // Initialize the output buffer as an empty string
    out[0] = '\0';

    char c;
    uint16_t size = fifo_size(stfifo);

    for (int i = 0; i < size; i++)
    {
        c = fifo_pop(stfifo);
        out[i] = c;
    }

    // Add null-terminator at the end
    out[size] = '\0';

    return EXIT_SUCESS;
}
//number of elements in our fifo
//
uint8_t fifo_size(ST_FIFO *stfifo)
{

	if(stfifo->fifo_dead)
	{
		return EPERM;
	}

	return (stfifo->index_write-stfifo->index_read);
}
