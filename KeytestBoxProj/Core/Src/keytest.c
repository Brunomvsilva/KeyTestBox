#include "keytest.h"

#include "usart.h"
#include "spi.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "gpio.h"
#include "boxUserConfig.h"
#include "MAX1032.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "fifo.h"
#include "cmdparser.h"
#include "mqtt_com.h"


extern osThreadId KeyTestTaskHandle;
extern osThreadId SecondTestTaskHandle;

extern xQueueHandle SmacOperationsQueue;
extern xQueueHandle Smac2OperationsQueue;

extern mqtt_client_t *mqtt_client_instance;

extern UART_HandleTypeDef huart4;	//SMAC 1
extern UART_HandleTypeDef huart5;	//SMAC 2
extern UART_HandleTypeDef huart3;	//LIN

extern CAN_HandleTypeDef hcan1;		//CAN 1

extern TIM_HandleTypeDef htim4;		//ENCODEER SMAC 1
extern TIM_HandleTypeDef htim3;     //ENCODEER SMAC 2

extern TIM_HandleTypeDef htim13;	//Frequency KEYTEST 1
extern TIM_HandleTypeDef htim14;	//Frequency KEYTEST 2


extern s_KeyTest* st_Keytest;

uint8_t SmacBuffer[BUFFER_SIZE];
extern ST_FIFO SmacRecvFIFO;
extern uint8_t accept_chars_smac;

uint8_t UUT_LIN_RxData[16] = "";

extern char outputTestData[SIZE_OUTPUT_BUFFER];

extern s_MAX1032 adc1;
extern s_MAX1032 adc2;

s_UserCANFilters CAN_FiltersList;
s_UserLINFilters LIN_FiltersList;



char TestParamsSmac [NUM_PARAMS][NUM_PARAM_ELEMENTS][PARAMS_SIZE] = {
		{"#RAPID POSN?\n", SMAC_DEFAULT_RPOS},
		{"#ACCELERATION?\n", SMAC_DEFAULT_ACCELLERATION},
		{"#VELOCITY?\n", SMAC_DEFAULT_VELOCITY},
		{"#TEST FORCE?\n", SMAC_DEFAULT_TESTFORCE},
		{"#MAX FORCE FWD?\n", SMAC_DEFAULT_MAXFORCE_FWD},
		{"#TEST INCREMENT?\n", SMAC_DEFAULT_TEST_INCREMENT},
		{"#WAIT INCREMENT?\n", SMAC_DEFAULT_WAIT_INCREMENT},
		{"#FST?\n", SMAC_DEFAULT_FST},
		{"#DIRECTION?\n", SMAC_DEFAULT_DIRECTION},
		{"#MAX FORCE RWD?\n", SMAC_DEFAULT_MAXFORCE_RWD}
};



void KeyTestInit(s_KeyTest* keytest_h, char Input[NUM_PARAMS][NUM_PARAM_ELEMENTS][PARAMS_SIZE], uint32_t freq_hz, char* p_out)
{


	if(xTaskGetCurrentTaskHandle() == KeyTestTaskHandle)
	{
		//INIT OF KEYTEST on SMAC 1
		keytest_h->SmacPort = &huart4;
		keytest_h->Encoder = &htim4;
		keytest_h->FreqTimer = &htim13;
		keytest_h->p_output = outputTestData;
		keytest_h->UUT_Protocol = NOT_DEFINED_PROTOCOL;

		//Init SMAC RxBuffer
		keytest_h->SmacRecvFIFO = &SmacRecvFIFO;
		keytest_h->SmacRecvFIFO->fifo = SmacBuffer;
		fifo_init(keytest_h->SmacRecvFIFO);
	}

	//Common Init Settings for Keytests on SMAC 1 and SMAC 2
	keytest_h->p_params = (char (*)[NUM_PARAM_ELEMENTS][PARAMS_SIZE])Input;
	keytest_h->LinPort = &huart3;
	keytest_h->CanPort = &hcan1;
	keytest_h->p_adc = &adc1;
	keytest_h->adcChannel = 0;
	keytest_h->TestFrequency_Hz = freq_hz;

	//Init Adcs with channel 0
	MAX1032_Init(&adc1, &hspi3, GPIOD, GPIO_PIN_4, 0);
	MAX1032_Init(&adc2, &hspi4, GPIOE, GPIO_PIN_4, 0);

	strcpy(keytest_h->p_output, "");

	HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

	if(HAL_CAN_Start(keytest_h->CanPort) != HAL_OK)
		Error_Handler();

	updateTestTimer(freq_hz);

	keytest_h->status = S_READY;
}



uint8_t KeyTestSetParams(s_KeyTest* keytest_h, char ParamToSet[20])
{

	//Run through Params List
	for(int i = 0; i < NUM_PARAMS; i++)
	{
		//Set That param
		if(strcmp(ParamToSet, keytest_h->p_params[i][0]) == 0)
		{
			HAL_UART_Transmit(keytest_h->SmacPort, (uint8_t*)keytest_h->p_params[i][1], strlen(keytest_h->p_params[i][1]), HAL_MAX_DELAY);
			return 1;
		}
	}

	return 0;
}


/*/////////////////////////
 *
 *   MQTT CMD CALLBACKS
 *
 */////////////////////////

extern Command KeytestBox_cmd_list[NUM_CMDS];
extern char last_in[64];
extern char last_cmd_used;

int Mqtt_Disconnect_cb (int argc, char* argv[])
{
	//If user inputs any argument
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(mqtt_client_is_connected(mqtt_client_instance))
		mqtt_disconnect(mqtt_client_instance);

	return 0;
}


int StartTest_cb (int argc, char* argv[])
{

	uint8_t operation = OP_START_TEST;

	//If number of (arguments+commands) different than 2 or 3
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	//Add Test to queue only if there is no test already running
	if(st_Keytest->status == S_READY)
	{
		st_Keytest->enabled = 1;
		Mqtt_AddSendOperation(LOG_TOPIC, "Starting Test\r\n");
		xQueueSend(SmacOperationsQueue, &operation, portMAX_DELAY);

	}

	return 0;
}

int MoveRelative_cb (int argc, char* argv[])
{
	char send[16] = "mr";

	//If number of (arguments+commands) different than 3
	if(argc != 2)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	strcat(send, argv[1]);
	strcat(send, ",go\r");

	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)send, strlen(send), HAL_MAX_DELAY);

	Mqtt_AddSendOperation(LOG_TOPIC, "Smac MR Sent\r\n");

	return 0;

}

int MoveHome_cb (int argc, char* argv[])
{

	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"\r", 1, HAL_MAX_DELAY);
	HAL_Delay(100);
	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"gh\r", 4, HAL_MAX_DELAY);

	Mqtt_AddSendOperation(LOG_TOPIC, "Smac Home Sent\r\n");

	return 0;
}

int ResetSmacMotor_cb (int argc, char* argv[])
{
	uint8_t esc = 0x1b;
	uint8_t operation = OP_START_TEST;

	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	HAL_UART_Transmit(st_Keytest->SmacPort, &esc, 1, HAL_MAX_DELAY);


	st_Keytest->enabled = 0;

	xQueueSendToFront(SmacOperationsQueue, &operation, portMAX_DELAY);


	return 0;
}

int setAllSmacParams_cb (int argc, char* argv[])
{
	if(argc != (NUM_PARAMS + 1))
		return UCMD_NUM_ARGS_NOT_VALID;

	//If Smac is being used do nothing
	if(st_Keytest->status != S_READY)
		return 0;

	for(int i = 0; i < NUM_PARAMS; i++)
	{
		strcpy(st_Keytest->p_params[i][1], "");
		strcat(argv[i+1], "\r");
		strcpy(st_Keytest->p_params[i][1], argv[i+1]);
	}

	Mqtt_AddSendOperation(LOG_TOPIC, "SetParam Done\r\n");
	return 0;
}

int setSmacParam_cb (int argc, char* argv[])
{
	if(argc != 3)
		return UCMD_NUM_ARGS_NOT_VALID;

	uint8_t index = atoi(argv[1]) - 1;

	if(st_Keytest->status != S_READY)
		return 0;

	strcpy(st_Keytest->p_params[index][1], "");
	strcat(argv[2], "\r");
	strcpy(st_Keytest->p_params[index][1], argv[2]);

	Mqtt_AddSendOperation(LOG_TOPIC, "SetParam Done\r\n");
	return 0;
}


int ChangeTimerFreq_cb (int argc, char* argv[])
{

	if(st_Keytest->status != S_READY)
		return 0;

	if(argc != 2)
		return UCMD_NUM_ARGS_NOT_VALID;

	uint16_t DesiredFreq = atoi(argv[1]);

	if(DesiredFreq >= 5000)
		return UCMD_ARGS_NOT_VALID;

	updateTestTimer(DesiredFreq);

	Mqtt_AddSendOperation(LOG_TOPIC, "Updated frequency\n");

	return 0;

}


int ReadAdc_cb (int argc, char* argv[])
{
	char aux[16] = "";
	float sample;
	uint8_t adcNum = atoi(argv[1]);
	uint8_t adcChannel = atoi(argv[2]);

	if(adcNum < 1 || adcNum > 2)
		return UCMD_ARGS_NOT_VALID;

	if(adcChannel < 0 || adcChannel > 2)
		return UCMD_ARGS_NOT_VALID;

	//If smac is in a test, do nothing
	if(st_Keytest->status != S_READY)
		return 0;

	if(adcNum == 1)
	{
		MAX1032_Init(&adc1, &hspi3, GPIOD, GPIO_PIN_4, adcChannel);
		sample = ReadADC_Polling(&adc1, adcChannel);
	}
	else
	{
		MAX1032_Init(&adc2, &hspi4, GPIOE, GPIO_PIN_4, adcChannel);
		sample = ReadADC_Polling(&adc2, adcChannel);
	}

	sprintf(aux,"%.3f V", sample);
	Mqtt_AddSendOperation(LOG_TOPIC, aux);

	return 0;
}

int ResetEncoder_cb (int argc, char* argv[])
{
	if(argc != 2)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	if(!strcmp(argv[1], "1"))
	{
		htim4.Instance->CNT = 0;
	}
	else
	if(!strcmp(argv[1], "2"))
	{
		htim3.Instance->CNT = 0;
	}
	else
		return UCMD_ARGS_NOT_VALID;

	Mqtt_AddSendOperation(LOG_TOPIC, "Reseted Encoder\r\n");
	return 0;

}

int ReadEncoder_cb (int argc, char* argv[])
{
	char aux[20] = "";
	int16_t sample;

	if(argc != 2)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	if(!strcmp(argv[1], "1"))
	{
		sample = __HAL_TIM_GET_COUNTER(&htim4);
		sprintf(aux,"Enc1 = %d", sample);
		Mqtt_AddSendOperation(LOG_TOPIC, aux);
	}
	else
	if(!strcmp(argv[1], "2"))
	{
		sample = __HAL_TIM_GET_COUNTER(&htim3);
		sprintf(aux,"Enc2 = %d", sample);
		Mqtt_AddSendOperation(LOG_TOPIC, aux);
	}
	else
		return UCMD_ARGS_NOT_VALID;

	return 0;
}

int TellPosition_cb (int argc, char* argv[])
{
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	//Enable Smac Message Reception on ISR
	accept_chars_smac = 1;
	//Send TP to selected SMAC
	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"tp\r", strlen("tp\r"), HAL_MAX_DELAY);


	return 0;
}

int TellTorque_cb (int argc, char* argv[])
{
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	//Enable Smac Message Reception on ISR
	accept_chars_smac = 1;
	//Send TP to selected SMAC
	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"tq\r", strlen("tq\r"), HAL_MAX_DELAY);


	return 0;
}


//"canfilter (ID) (isExt) (FIFO) (DLC) (StartBit) (numBits)\r\n",
int CanConfigFilter_cb (int argc, char* argv[])
{
	uint32_t ID, aux;
	uint8_t isExtended;
	uint8_t fifo;
	uint8_t startBit, numBits, DLC;
	CAN_FilterTypeDef canfilterconfig0, canfilterconfig1;
	s_CANFilter newFilter;


	if(argc != 7)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	if(!strcmp(argv[2], "std"))
	{
		isExtended = CAN_ID_STD;
	}
	else
	if(!strcmp(argv[2], "ext"))
		isExtended = CAN_ID_EXT;
	else
		return UCMD_ARGS_NOT_VALID;


	ID = strtol(argv[1], NULL, 16);
	fifo = atoi(argv[3]);
	DLC = atoi(argv[4]);
	startBit = atoi(argv[5]);
	numBits = atoi(argv[6]);
	aux = ID;

	if(numBits <= 0 || numBits > 32)
		return UCMD_ARGS_NOT_VALID;

	newFilter.ReadDataInfo.ID = ID;
	newFilter.ReadDataInfo.is_extendedID = isExtended;
	newFilter.targetFIFO = fifo;
	newFilter.ReadDataInfo.numBits = numBits;
	newFilter.ReadDataInfo.startBit = startBit;
	newFilter.ReadDataInfo.NumDataBytes = DLC;    // DLC is not really needed but might be in future versions


	switch (fifo)
	{
		case CAN_FIFO0:
			 //Check if there is already a filter with this ID in the List of FIFO 0
			 for(int i = 0; i < CAN_FiltersList.numFiltersFIFO0; i++)
			 {
				 if((ID == CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.ID) && (isExtended == CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.is_extendedID))
				 {
					 //Update DLC, numBits and start bit of the filter with the values from command
					 CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.NumDataBytes = DLC;
					 CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.numBits = numBits;
					 CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.startBit = startBit;
					 Mqtt_AddSendOperation(LOG_TOPIC, "Filter Updated\r\n");
					 return 0;
				 }
			 }

			//If it reaches here then this Filter does not exist in the List yet
			//Add new Filter to List of FIFO0
			 if(isExtended)
			 {
				 aux <<= 3;
				 canfilterconfig0.FilterIdHigh = (((uint32_t)aux & 0xFFFF0000U)>>16);  // Set the ID you want to allow (0x102)
				 canfilterconfig0.FilterIdLow =  ((uint32_t)aux & 0x0000FFFFU);
				 canfilterconfig0.FilterMaskIdHigh = 0xFFFFU;  // Set mask to accept all bits in ID
				 canfilterconfig0.FilterMaskIdLow = 0xFFF8U;
			 }
			 else
			 {
				 canfilterconfig0.FilterIdHigh = ID << 5;  // Set the ID you want to allow (0x102)
				 canfilterconfig0.FilterIdLow = 0x0000;
				 canfilterconfig0.FilterMaskIdHigh = 0x7FF << 5;  // Set mask to accept all bits in ID
				 canfilterconfig0.FilterMaskIdLow = 0x0000;
			 }

			 canfilterconfig0.FilterActivation = CAN_FILTER_ENABLE;
			 canfilterconfig0.FilterBank = CAN_FiltersList.numFiltersFIFO0;  // Choose a suitable filter bank
		     canfilterconfig0.FilterFIFOAssignment = CAN_RX_FIFO0;
			 canfilterconfig0.FilterMode = CAN_FILTERMODE_IDMASK;
			 canfilterconfig0.FilterScale = CAN_FILTERSCALE_32BIT;
			 canfilterconfig0.SlaveStartFilterBank = 0;	//NOT USED?

			 if(CAN_FiltersList.numFiltersFIFO0 == MAX_NUM_CAN_FILTERS)
			 {
				 Mqtt_AddSendOperation(ERROR_TOPIC, "Error FIFO0 Filter List Full");
				 return 0;
			 }

			 CAN_FiltersList.FIFO0_Filters[CAN_FiltersList.numFiltersFIFO0] = newFilter;
			 CAN_FiltersList.numFiltersFIFO0++;

			 HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig0);
			 Mqtt_AddSendOperation(LOG_TOPIC, "Filter Added\r\n");
			 break;

		case CAN_FIFO1:
			//Check if there is already a filter with this ID in the List of FIFO 0
			for(int i = 0; i < CAN_FiltersList.numFiltersFIFO1; i++)
			{
				if((ID == CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.ID) && (isExtended == CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.is_extendedID))
				{
					//Update DLC, numBits and start bit of the filter with the values from command
					CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.NumDataBytes = DLC;
					CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.numBits = numBits;
					CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.startBit = startBit;
					Mqtt_AddSendOperation(LOG_TOPIC, "Filter Updated\r\n");
					return 0;
				}
			}

			//If it reaches here then this Filter does not exist in the List yet
			//Add new Filter to List of FIFO1
			if(isExtended)
			{
				aux <<= 3;
				canfilterconfig1.FilterIdHigh = ((uint32_t)aux & 0xFFFF0000U);  // Set the ID you want to allow (0x102)
				canfilterconfig1.FilterIdLow =  ((uint32_t)aux & 0x0000FFFFU);
				canfilterconfig1.FilterMaskIdHigh = 0xFFFFU;  // Set mask to accept all bits in ID
				canfilterconfig1.FilterMaskIdLow = 0xFFF8U;
			}
			else
			{
				canfilterconfig1.FilterIdHigh = ID << 5;  // Set the ID you want to allow (0x102)
				canfilterconfig1.FilterIdLow = 0x0000;
				canfilterconfig1.FilterMaskIdHigh = 0x7FF << 5;  // Set mask to accept all bits in ID
				canfilterconfig1.FilterMaskIdLow = 0x0000;
			}

			canfilterconfig1.FilterActivation = CAN_FILTER_ENABLE;
			canfilterconfig1.FilterBank = CAN_FiltersList.numFiltersFIFO1;  // Choose a suitable filter bank
			canfilterconfig1.FilterFIFOAssignment = CAN_RX_FIFO1;
			canfilterconfig1.FilterMode = CAN_FILTERMODE_IDMASK;
			canfilterconfig1.FilterScale = CAN_FILTERSCALE_32BIT;
			canfilterconfig1.SlaveStartFilterBank = 0;	//NOT USED?

			if(CAN_FiltersList.numFiltersFIFO1 == MAX_NUM_CAN_FILTERS)
			{
				Mqtt_AddSendOperation(ERROR_TOPIC, "Error FIFO1 Filter List Full");
				return 0;
			}

			CAN_FiltersList.FIFO1_Filters[CAN_FiltersList.numFiltersFIFO1] = newFilter;
			CAN_FiltersList.numFiltersFIFO1++;

			HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig1);
			Mqtt_AddSendOperation(LOG_TOPIC, "Filter Added\r\n");
			break;

		default:
			return UCMD_ARGS_NOT_VALID;
			break;
	}

	return 0;

}

//linfilter (ID) (DLC) (StartBit) (numBits)\r\n"
int LinConfigFilter_cb (int argc, char* argv[])
{
	uint16_t ID;
	uint8_t NumDataBytes;
	uint8_t startBit;
	uint8_t numBits;

	if(argc != 5)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	ID = strtol(argv[1], NULL, 16);
	NumDataBytes = atoi(argv[2]);
	startBit = atoi(argv[3]);
	numBits = atoi(argv[4]);

	//Check if there is already a Filter with this ID
	for(int i = 0; i < LIN_FiltersList.numLINFilters; i++)
	{
		if(LIN_FiltersList.LINFilters[i].ReadDataInfo.ID == ID)
		{
			//Only update Filter Info and return
			LIN_FiltersList.LINFilters->ReadDataInfo.NumDataBytes = NumDataBytes;
			LIN_FiltersList.LINFilters->ReadDataInfo.startBit = startBit + 24; //Add 24 bits because first 3 bytes are Break byte, sync Byte and PID byte
			LIN_FiltersList.LINFilters->ReadDataInfo.numBits = numBits;
			Mqtt_AddSendOperation(LOG_TOPIC, "Filter Updated\r\n");
			return 0;
		}
	}

	if(LIN_FiltersList.numLINFilters == MAX_NUM_LIN_FILTERS)
	{
		Mqtt_AddSendOperation(ERROR_TOPIC, "LIN Filter List FULL");
		return 0;
	}


	LIN_FiltersList.LINFilters->ReadDataInfo.ID = ID;
	LIN_FiltersList.LINFilters->ReadDataInfo.NumDataBytes = NumDataBytes;
	LIN_FiltersList.LINFilters->ReadDataInfo.startBit = startBit + 24;
	LIN_FiltersList.LINFilters->ReadDataInfo.numBits = numBits;
	LIN_FiltersList.numLINFilters++;
	Mqtt_AddSendOperation(LOG_TOPIC, "Filter Added\r\n");

	return 0;

}

int CanRemoveFilters_cb (int argc, char* argv[])
{
	CAN_FilterTypeDef canfilterconfig;

	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	for(int i = 0; i < CAN_FiltersList.numFiltersFIFO0; i++)
	{
		canfilterconfig.FilterActivation = CAN_FILTER_DISABLE;
		canfilterconfig.FilterBank = i;
		canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0;
		canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
		canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
		canfilterconfig.SlaveStartFilterBank = 0;

		HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);

		CAN_FiltersList.numFiltersFIFO0 = 0;
	}


	for(int i = 0; i < CAN_FiltersList.numFiltersFIFO1; i++)
	{
		canfilterconfig.FilterActivation = CAN_FILTER_DISABLE;
		canfilterconfig.FilterBank = i;
		canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO1;
		canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
		canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
		canfilterconfig.SlaveStartFilterBank = 0;

		HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);

		CAN_FiltersList.numFiltersFIFO1 = 0;

	}

	//If CAN Protocol was being used. Set protocol to not defined
	if((st_Keytest->UUT_Protocol == CAN_PROTOCOL) && (st_Keytest->isCanFIFO0_Active || st_Keytest->isCanFIFO1_Active))
		st_Keytest->UUT_Protocol = NOT_DEFINED_PROTOCOL;


	//Clear Interest IDs Buffers // Only Id and startBit clear is needed
	for(int i = 0; i < st_Keytest->numInterestIDs; i++)
	{
		st_Keytest->UUT_InterestIDsInfo[i].ID = 0;
		st_Keytest->UUT_InterestIDsInfo[i].startBit = 0;
	}
	st_Keytest->numInterestIDs = 0;

	Mqtt_AddSendOperation(LOG_TOPIC, "Filters Removed\r\n");

	return 0;
}

int LinRemoveFilters_cb (int argc, char* argv[])
{
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
			return 0;


//	for(int i = 0; i < LIN_FiltersList.numLINFilters; i++)
//	{
//		LIN_FiltersList.LinFiltersActive[i] = 0;
//	}


	LIN_FiltersList.numLINFilters = 0;

	if(st_Keytest->UUT_Protocol == LIN_PROTOCOL)
		st_Keytest->UUT_Protocol = NOT_DEFINED_PROTOCOL;

	//Clear Interest IDs Buffers // Only Id and startBit clear is needed
	for(int i = 0; i < st_Keytest->numInterestIDs; i++)
	{
		st_Keytest->UUT_InterestIDsInfo[i].ID = 0;
		st_Keytest->UUT_InterestIDsInfo[i].startBit = 0;
	}
	st_Keytest->numInterestIDs = 0;

	Mqtt_AddSendOperation(LOG_TOPIC, "Filters Removed\r\n");

	return 0;
}

//"setpiece (Protocol) (ID) (Ext)\r\n",
int SetPiece_cb (int argc, char* argv[])
{
	uint32_t ID;
	bool ID_exists = 0;
	uint8_t isExtended;


	if(argc != 4 && argc != 3)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	if(st_Keytest->numInterestIDs == CAN_MAX_NUM_INTEREST_IDS)
	{
		Mqtt_AddSendOperation(ERROR_TOPIC, "Max CAN Num Interest Messages Defined");
		return 0;
	}

	if(argc == 4)
	{
		if(!strcmp(argv[3], "std"))
		{
			isExtended = CAN_ID_STD;
		}
		else
		if(!strcmp(argv[3], "ext"))
			isExtended = CAN_ID_EXT;
		else
			return UCMD_ARGS_NOT_VALID;
	}

	ID = strtol(argv[2], NULL, 16);
	bool protocol;

	if(!strcmp(argv[1], "can"))
	{
		protocol = CAN_PROTOCOL;
		//Check if there is a CAN filter for the ID in FIFO 0 and 1
		for(int i = 0; i < CAN_FiltersList.numFiltersFIFO0; i++)
		{
			if((CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.ID == ID) && (CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.is_extendedID == isExtended))
			{
				if(st_Keytest->numInterestIDs < CAN_MAX_NUM_INTEREST_IDS)
				{
					//Verifies if this Interest Message wasnt already defined
					for(int j = 0; j < st_Keytest->numInterestIDs; j++)
					{
						if((st_Keytest->UUT_InterestIDsInfo[j].ID == CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.ID)
							&& (st_Keytest->UUT_InterestIDsInfo[j].startBit == CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.startBit))
						{
							Mqtt_AddSendOperation(ERROR_TOPIC, "Already defined Interest Message with same ID and startBit");
							return 0;
						}
					}
					st_Keytest->isCanFIFO0_Active = 1;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].numBits = CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.numBits;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].startBit = CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.startBit;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].NumDataBytes = CAN_FiltersList.FIFO0_Filters[i].ReadDataInfo.NumDataBytes;
					st_Keytest->numInterestIDs++;
					ID_exists = 1;
				}
				else
					ID_exists = 0;

				goto END_SET_PIECE_SEARCH;
			}

		}
		for(int i = 0; i < CAN_FiltersList.numFiltersFIFO1; i++)
		{
			if((CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.ID == ID) && (CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.is_extendedID == isExtended))
			{
				if(st_Keytest->numInterestIDs < CAN_MAX_NUM_INTEREST_IDS)
				{
					//Verifies if this Interest Message wasnt already defined
					for(int j = 0; j < st_Keytest->numInterestIDs; j++)
					{
					    if((st_Keytest->UUT_InterestIDsInfo[j].ID == CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.ID)
						&& (st_Keytest->UUT_InterestIDsInfo[j].startBit == CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.startBit))
					    {
					    	Mqtt_AddSendOperation(ERROR_TOPIC, "Already defined Interest Message with same ID and startBit");
					    	return 0;
					    }
					}

					st_Keytest->isCanFIFO1_Active = 1;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].numBits = CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.numBits;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].startBit = CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.startBit;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].NumDataBytes = CAN_FiltersList.FIFO1_Filters[i].ReadDataInfo.NumDataBytes;
					st_Keytest->numInterestIDs++;
					ID_exists = 1;
				}
				else
					ID_exists = 0;

				goto END_SET_PIECE_SEARCH;
			}

		}

	}
	else
	if(!strcmp(argv[1], "lin"))
	{
		protocol = LIN_PROTOCOL;
		for(int i = 0; i < LIN_FiltersList.numLINFilters; i++)
		{
			if((LIN_FiltersList.LINFilters[i].ReadDataInfo.ID == ID))
			{
				if(st_Keytest->numInterestIDs < LIN_MAX_NUM_INTEREST_IDS)
				{
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].NumDataBytes = LIN_FiltersList.LINFilters[i].ReadDataInfo.NumDataBytes;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].numBits = LIN_FiltersList.LINFilters[i].ReadDataInfo.numBits;
					st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs].startBit = LIN_FiltersList.LINFilters[i].ReadDataInfo.startBit;
					st_Keytest->numInterestIDs++;
					ID_exists = 1;
				}

				goto END_SET_PIECE_SEARCH;
			}

		}
	}
	else
		return UCMD_ARGS_NOT_VALID;

	END_SET_PIECE_SEARCH:
		if(ID_exists)
		{
			st_Keytest->UUT_Protocol = protocol;
			st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs -1].ID = ID;
			st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs -1].is_extendedID = isExtended; //If its LIN this value will later be ignored
			Mqtt_AddSendOperation(LOG_TOPIC, "Piece is Setup\r\n");
		}
		else
		{
			if(st_Keytest->numInterestIDs == 0)
				st_Keytest->UUT_Protocol = NOT_DEFINED_PROTOCOL;

			Mqtt_AddSendOperation(ERROR_TOPIC, "Filter for this ID doesnt exist or InterestIDs List full\n");
		}


	return 0;
}

int SelectSmac_cb (int argc, char* argv[])
{
	if(argc != 2)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	if(!strcmp(argv[1], "1"))
		st_Keytest->SmacPort = &huart4;
	else
	if(!strcmp(argv[1], "2"))
		st_Keytest->SmacPort = &huart5;
	else
		return UCMD_ARGS_NOT_VALID;

	Mqtt_AddSendOperation(LOG_TOPIC, "Smac Selected\r\n");

	return 0;
}

int SelectEncoder_cb (int argc, char* argv[])
{
	if(argc != 2)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	if(!strcmp(argv[1], "1"))
		st_Keytest->Encoder = &htim4;
	else
	if(!strcmp(argv[1], "2"))
		st_Keytest->Encoder = &htim3;
	else
		return UCMD_ARGS_NOT_VALID;

	Mqtt_AddSendOperation(LOG_TOPIC, "Encoder Selected\r\n");

	return 0;
}

int SelectAdc_cb (int argc, char* argv[])
{
	if(argc != 3)
		return UCMD_NUM_ARGS_NOT_VALID;

	uint8_t adcNum = atoi(argv[1]);
	uint8_t adcChannel = atoi(argv[2]);

	if(st_Keytest->status != S_READY)
		return 0;

	if(adcNum < 1 || adcNum > 2)
		return UCMD_ARGS_NOT_VALID;

	if(adcChannel < 0 || adcChannel > 2)
		return UCMD_ARGS_NOT_VALID;

	if(adcNum == 1)
	{
		st_Keytest->adcChannel = adcChannel;
		st_Keytest->p_adc = &adc1;
		MAX1032_Init(&adc1, &hspi3, GPIOD, GPIO_PIN_4, adcChannel);
	}
	else
	{
		st_Keytest->adcChannel = adcChannel;
		st_Keytest->p_adc = &adc2;
		MAX1032_Init(&adc2, &hspi4, GPIOE, GPIO_PIN_4, adcChannel);
	}

	Mqtt_AddSendOperation(LOG_TOPIC, "ADC Selected\r\n");

	return 0;
}

int MotorOn_cb (int argc, char* argv[])
{
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	//Send TP to selected SMAC
	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"mn\r", strlen("mn\r"), HAL_MAX_DELAY);

	Mqtt_AddSendOperation(LOG_TOPIC, "Sent Motor On\r\n");


	return 0;
}

int MotorOff_cb (int argc, char* argv[])
{
	if(argc != 1)
		return UCMD_NUM_ARGS_NOT_VALID;

	if(st_Keytest->status != S_READY)
		return 0;

	//Send TP to selected SMAC
	HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"mf\r", strlen("mf\r"), HAL_MAX_DELAY);

	Mqtt_AddSendOperation(LOG_TOPIC, "Sent Motor Off\r\n");

	return 0;
}

int Last_cmd_cb (int argc, char* argv[])
{
	last_cmd_used = 1;
	return ucmd_parse(KeytestBox_cmd_list, UCMD_DEFAULT_DELIMETER, last_in);
}

//int Help_cb (int argc, char* argv[])
//{
//	char msg[1024] = "";
//	char aux[64] = "";
//
//	for(Command* p = KeytestBox_cmd_list; p->fn != NULL; p++ )
//	{
//		strcpy(aux, (char*)p->help);
//		strcat(msg, aux);
//	}
//
//	Mqtt_AddSendOperation(LOG_TOPIC, msg);
//	return 0;
//}

int idle_cb (int argc, char* argv[])
{
	return 0;
}


/*/////////////////////////
 *
 *    OTHER AUX FUNCTIONS
 *
 */////////////////////////

HAL_StatusTypeDef SMAC_SendMessage(UART_HandleTypeDef* smac, uint8_t *pData)
{
	HAL_StatusTypeDef err;

	err = HAL_UART_Transmit(smac, pData, strlen((char*)pData), HAL_MAX_DELAY);

	return err;
}

void StartEncoder(s_KeyTest* keytest_h)
{
	HAL_TIM_Encoder_Start(keytest_h->Encoder, TIM_CHANNEL_ALL);
}

void StopEncoder(s_KeyTest* keytest_h)
{
	HAL_TIM_Encoder_Stop(keytest_h->Encoder, TIM_CHANNEL_ALL);
}

void StartTestTimer(s_KeyTest* keytest_h)
{
	HAL_TIM_Base_Start_IT(keytest_h->FreqTimer);
}

void StopTestTimer(s_KeyTest* keytest_h)
{
	HAL_TIM_Base_Stop_IT(keytest_h->FreqTimer);
}

void ClearTestTimer(s_KeyTest* keytest_h)
{
	keytest_h->FreqTimer->Instance->CNT = 0;
}

int16_t ReadEncoder(s_KeyTest* keytest_h)
{
	int16_t counter = __HAL_TIM_GET_COUNTER(keytest_h->Encoder);

	return counter;
}

void updateTestTimer(uint16_t timerFreq)
{
	uint32_t newPrescaler = 0;
	uint32_t newCounter = 0;

	float calc = (TIMER_CLK_FREQUENCY_MHZ * 1e6)/timerFreq;
	//Calculate timer Prescaler and Counter


	//Use the lowest prescaler possible
	//for cicle should never run more than 10 times unless the chosen frequency is too low (less than 200 Hz)
	for(newPrescaler = 0; newPrescaler <= 20; newPrescaler++)
	{
		calc = -1 + (calc/(newPrescaler + 1));
		newCounter = calc;

		if(newCounter > 0 && newCounter <= 65535)
			break;

	}

	st_Keytest->TestFrequency_Hz = timerFreq;
	st_Keytest->FreqTimer->Instance = TIM13;
	st_Keytest->FreqTimer->Init.Prescaler = newPrescaler;
	st_Keytest->FreqTimer->Init.CounterMode = TIM_COUNTERMODE_UP;
	st_Keytest->FreqTimer->Init.Period = newCounter;
	st_Keytest->FreqTimer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	st_Keytest->FreqTimer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(st_Keytest->FreqTimer) != HAL_OK)
	{
	  Error_Handler();
	}

	return;

}

void SendSmacHome(UART_HandleTypeDef* huart)
{
	SMAC_SendMessage(huart, (uint8_t*)"gh\r");

}

uint32_t getNumOutputBytes()
{
  uint32_t count = 0;
  uint32_t i = 0;

  while(outputTestData[i] != '\0')
  {
	  count++;
	  i++;
  }
  return count;
}


//returns desired bits of an array of bytes
//This function only works correctly if the values startBit and numBits are a power of 2
uint32_t readBits(uint8_t Data[8], uint8_t startBit, uint8_t numBits)
{
    uint32_t result = 0;

    // Calculate the starting byte and bit offset
    uint8_t startByte = startBit / 8;
    uint8_t bitOffset = startBit % 8;

    // Calculate the number of bytes to read
    uint8_t numBytes = (numBits + bitOffset + 7) / 8;

    // Extract bits from the array and construct the result
    for (int8_t i = numBytes - 1; i >= 0; --i) {
        result |= (uint32_t)(Data[startByte + i] >> bitOffset) << (8 * (numBytes - 1 - i));
        if (bitOffset + numBits > 8) {
            bitOffset = (bitOffset + numBits) % 8;
        }
    }

    if(numBits == 32)
        return result;

    result &= ((1U << numBits)-1);

    return result;
}

//Big endian
//uint32_t readBits(uint8_t Data[8], uint8_t startBit, uint8_t numBits)
//{
//    uint32_t result = 0;
//
//    // Calculate the starting byte and bit offset
//    uint8_t startByte = startBit / 8;
//    uint8_t bitOffset = startBit % 8;
//
//    // Calculate the number of bytes to read
//    uint8_t numBytes = (numBits + bitOffset + 7) / 8;
//
//    // Extract bits from the array and construct the result
//    for (int8_t i = numBytes - 1; i >= 0; --i) {
//        result |= (uint32_t)(Data[startByte + i] >> (bitOffset)) << (8 * (numBytes - 1 - i));
//        if (bitOffset + numBits > 8) {
//            bitOffset = (bitOffset + numBits) % 8;
//        }
//    }
//
//    if(numBits == 32)
//        return result;
//
//    result &= ((1U << numBits) - 1);
//
//    return result;
//}


//Little Endian
//uint32_t readBits(uint8_t Data[8], uint8_t startBit, uint8_t numBits)
//{
//    uint32_t result = 0;
//
//    // Calculate the starting byte and bit offset
//    uint8_t startByte = startBit / 8;
//    uint8_t bitOffset = startBit % 8;
//
//    // Calculate the number of bytes to read
//    uint8_t numBytes = (numBits + bitOffset + 7) / 8;
//
//    // Extract bits from the array and construct the result
//    for (int8_t i = 0; i < numBytes; ++i) {
//        result |= (uint32_t)(Data[startByte + i] >> (bitOffset)) << (8 * i);
//        if (bitOffset + numBits > 8) {
//            bitOffset = (bitOffset + numBits) % 8;
//        }
//    }
//
//    if(numBits == 32)
//        return result;
//
//    result &= ((1U << numBits) - 1);
//
//    return result;
//}


