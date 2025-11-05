#include "stdlib.h"
#include "string.h"
#include "cmdparser.h"
#include "stdio.h"
#include "mqtt_com.h"
#include "keytest.h"

//static int last_cmd_argc = 0;
char last_in[64];
//static char** last_cmd_argv = NULL;
char last_cmd_used;


extern Command KeytestBox_cmd_list[NUM_CMDS];


int ucmd_parse(Command cmd_list[], const char* delim, const char* in)
{
    if (!in || strlen(in) == 0) return 0;
    if (!cmd_list) return UCMD_CMD_NOT_FOUND;

    delim = (delim) ? delim : UCMD_DEFAULT_DELIMETER; //if delim is defined -> keep it, if not define as default

    int retval = 0;     //return value

    char* s = (char*)malloc(strlen(in) + 1);    //alocates and copies input string
    strcpy(s, in);

    int argc = 0;
    char** argv = NULL;

    char* last;
    char* arg = strtok_r(s, delim, &last);

    while (arg) {
        argc++;
        argv = (char**)realloc(argv, argc * sizeof(*argv));
        argv[argc - 1] = arg;
        arg = strtok_r(NULL, delim, &last);
    }

    if (argc) {
        Command* c = NULL;
        for (Command* p = cmd_list; p->cmd; p++) {
            if (strcmp(p->cmd, argv[0]) == 0)
            {
                c = p;
                break;
            }
            if(p->fn == NULL)
                break;
        }
        if (c)
        {
            retval = c->fn(argc, argv);
            if(strcmp(c->cmd, "$" ) || last_cmd_used != 0)
            {
                strcpy(last_in, in);
            }
        }
        else {
            retval = UCMD_CMD_NOT_FOUND;
        }
    }
    last_cmd_used = 0;
    free(argv);
    free(s);

    return retval;
}



Command KeytestBox_cmd_list[NUM_CMDS] = {
    {
        "idle",
        "",
        (Command_cb) idle_cb,
    },
	{
	    "disconnect",
	    "disconnect\r\n",
	    (Command_cb) Mqtt_Disconnect_cb,
	},
	{
		"setallparams",
		"setallparams (index) (all parameter values)\r\n",
		(Command_cb) setAllSmacParams_cb,
	},
	{
		"setparam",
		"setparam (index) (value)\r\n",
		(Command_cb) setSmacParam_cb,
	},
	{
		"ms0",
		"ms0\r\n",
		(Command_cb) ResetSmacMotor_cb,
	},
	{
		"resetsmac",
		"resetsmac\r\n",
		(Command_cb) ResetSmacMotor_cb,
	},
	{
		"keytest",
		"keytest\r\n",
		(Command_cb) StartTest_cb,
	},
	{
		"stoptest",
		"stoptest\r\n",
		(Command_cb) ResetSmacMotor_cb,
	},
	{
	    "mr",
		"mr (counts)\r\n",
		(Command_cb) MoveRelative_cb,
	},
	{
		"gh",
		"gh\r\n",
		(Command_cb) MoveHome_cb,
	},
	{
		"readadc",
		"readadc (Adc Num) (Adc Channel)\r\n",
		(Command_cb) ReadAdc_cb,
	},
	{
		"resetencoder",
		"resetencoder (Enc num)\r\n",
		(Command_cb) ResetEncoder_cb,
	},
	{
		"readencoder",
		"readencoder (Enc num)\r\n",
		(Command_cb) ReadEncoder_cb,
	},
	{
		"selectencoder",
		"selectencoder (Enc num)\r\n",
		(Command_cb) SelectEncoder_cb,
	},
	{
		"canfilter",
		"canfilter (ID) (isExt) (fifo num) (DLC) (startBit) (numBits)\r\n",
		(Command_cb) CanConfigFilter_cb,
	},
	{
		"linfilter",
		"linfilter (ID) (DLC) (StartBit) (numBits)\r\n",
		(Command_cb) LinConfigFilter_cb,
	},
	{
		"canremovefilters",
		"canremovefilters\r\n",
		(Command_cb) CanRemoveFilters_cb,
	},
	{
		"linremovefilters",
		"linremovefilters\r\n",
		(Command_cb) LinRemoveFilters_cb,
	},
	{
		"setpiece",
		"setpiece (can/lin) (ID)\r\n",
		(Command_cb) SetPiece_cb,
	},
	{
		"selectsmac",
		"selectsmac (Smac num)\r\n",
		(Command_cb) SelectSmac_cb,
	},
	{
		"testfrequency",
		"testfrequency (frequecy)\r\n",
		(Command_cb) ChangeTimerFreq_cb,
	},
	{
		"selectencoder",
		"selectencoder (Enc num)\r\n",
		(Command_cb) SelectEncoder_cb,
	},
	{
		"selectadc",
		"selectadc (Adc num)(Adc Channel)\r\n",
		(Command_cb) SelectAdc_cb,
	},
	{
		"tp",
		"tp\r\n",
		(Command_cb) TellPosition_cb,
	},
	{
		"tq",
		"tq\r\n",
		(Command_cb) TellTorque_cb,
	},
	{
		"mf",
		"mf\r\n",
		(Command_cb) MotorOff_cb,
	},
	{
		"mn",
		"tq\r\n",
		(Command_cb) MotorOn_cb,
	},
	{
        "$",
        "$-executes last cmd\r\n",
        (Command_cb) Last_cmd_cb,
    },
//	{
//        "help",
//        "help\r\n",
//        (Command_cb) Help_cb,
//    },
    {
        "",
        "null list terminator",
        NULL
    },
};

//mf desliga o smac
//mn liga
// TP e TQ
