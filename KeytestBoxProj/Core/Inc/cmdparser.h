#ifndef INC_CMDPARSER_H_
#define INC_CMDPARSER_H_

#include "limits.h"
#include "stdint.h"

#define NUM_CMDS	30

#define UCMD_CMD_NOT_FOUND 					INT_MIN

#define UCMD_NUM_ARGS_NOT_VALID	    		INT_MIN+1

#define UCMD_ARGS_NOT_VALID   				INT_MIN+2

#define UCMD_CMD_LIST_NOT_FOUND 			INT_MIN+3

#define UCMD_CMD_LAST_CMD_LOOP				INT_MIN+4

#define UCMD_DEFAULT_DELIMETER 				" "


typedef int (*Command_cb)(int, char* []);

typedef struct Command
{
	const char *cmd;		//Command string
	const char *help;		//Help message for the Command
	Command_cb fn; 			//Command Callback
}Command;



int ucmd_parse(Command [], const char*, const char* in);


#endif /* INC_CMDPARSER_H_ */
