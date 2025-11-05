#ifndef INC_KEYTEST_H_
#define INC_KEYTEST_H_

#include "stdint.h"
#include "stdbool.h"
#include "usart.h"
#include "MAX1032.h"
#include "fifo.h"

#define OP_START_TEST		 	(1 << 0)
#define OP_STOP_TEST         	(1 << 1)
#define OP_TEST_SAMPLE     	 	(1 << 2)
#define OP_SMAC_MSG	   		 	(1 << 3)
#define OP_UPDATE_CAN_VALUE     (1 << 4)
#define OP_UPDATE_LIN_VALUE     (1 << 5)

#define NUM_PARAMS 10
#define NUM_PARAM_ELEMENTS 2
#define PARAMS_SIZE 20

#define CAN_PROTOCOL 0
#define LIN_PROTOCOL 1
#define NOT_DEFINED_PROTOCOL 2

#define CAN_FIFO0 0
#define CAN_FIFO1 1

#define MAX_NUM_CAN_FILTERS 5
#define MAX_NUM_LIN_FILTERS 3

#define LIN_HEADER_BYTES 4

typedef struct UUT_DataReadInfo
{
	uint32_t ID;
	uint8_t is_extendedID;
	uint8_t startBit;
	uint8_t numBits;
	uint8_t NumDataBytes;  //Data bytes of the full  CAN Message Received
}s_UUT_DataReadInfo;

typedef struct CANFilter
{
	bool targetFIFO;
	s_UUT_DataReadInfo ReadDataInfo;
} s_CANFilter;

typedef struct LINFilter
{
	s_UUT_DataReadInfo ReadDataInfo;
} s_LINFilter;

typedef struct UserLINFilters
{
	s_LINFilter LINFilters[MAX_NUM_LIN_FILTERS];
	//bool LinFiltersActive[MAX_NUM_LIN_FILTERS];
	uint8_t numLINFilters;
	bool LinRequestPending;

}s_UserLINFilters;

typedef struct UserCANFilters
{
	s_CANFilter FIFO0_Filters[MAX_NUM_CAN_FILTERS];
	s_CANFilter FIFO1_Filters[MAX_NUM_CAN_FILTERS];
	uint8_t numFiltersFIFO0;
	uint8_t numFiltersFIFO1;

}s_UserCANFilters;


typedef enum
{
  S_BUSY              = 0x01U,   //Esperar parametros smac INPUT
  S_READY			  = 0x02U,
  S_OUTPUT            = 0x03U
  //
} KeyTestStatus;

typedef struct KeyTestHandle
{

	UART_HandleTypeDef* SmacPort; 	 					//SMAC RS232 Port / Default is huart4
	TIM_HandleTypeDef* FreqTimer;						//Test Frequency Timer
	TIM_HandleTypeDef* Encoder;	     					//Encoder of the KeyTest
	UART_HandleTypeDef* LinPort;     	 				//LIN PORT
	CAN_HandleTypeDef* CanPort;		 					//CAN PORT
	s_MAX1032* p_adc; 									//Pointer to ADC instance
	uint8_t adcChannel;



	char (*p_params)[NUM_PARAM_ELEMENTS][PARAMS_SIZE];  //Pointer to Test Params Array

	ST_FIFO* SmacRecvFIFO;                       		//SMAC RxBuffer

	uint32_t TestFrequency_Hz;

	KeyTestStatus status;

	//Keytest Internal Control Variables
	bool enabled;

	uint8_t UUT_Protocol;				            // 1 = CAN; 0 = LIN
	bool isCanFIFO0_Active;
	bool isCanFIFO1_Active;
	uint32_t UUT_ReadValues[5];			 			// Received Value of Interest from piece with ID of interest
	s_UUT_DataReadInfo UUT_InterestIDsInfo[5];      //Gives info about Num bits to Read from CAN/LIN Message
	uint8_t numInterestIDs;
	char* p_output;									//Send MQTT MSG Pointer

}s_KeyTest;



void KeyTestInit(s_KeyTest* keytest_h, char Input[NUM_PARAMS][NUM_PARAM_ELEMENTS][PARAMS_SIZE], uint32_t freq_hz, char* p_out);

uint8_t KeyTestSetParams(s_KeyTest* keytest_h, char paramToSet[16]);

HAL_StatusTypeDef SMAC_SendMessage(UART_HandleTypeDef* smac, uint8_t *pData);

void ProcessSmacMessage(s_KeyTest* keytest_h);

void StartEncoder(s_KeyTest* keytest_h);

void StopEncoder(s_KeyTest* keytest_h);

int16_t ReadEncoder(s_KeyTest* keytest_h);

void StartTestTimer(s_KeyTest* keytest_h);

void StopTestTimer(s_KeyTest* keytest_h);

void ClearTestTimer(s_KeyTest* keytest_h);

void SendSmacHome(UART_HandleTypeDef* huart);

void updateTestTimer(uint16_t timerFreq);

uint32_t getNumOutputBytes();

uint32_t readBits(uint8_t Data[], uint8_t startBit, uint8_t numBits);

/*
 * Command Callbacks
 */

int Mqtt_Disconnect_cb (int, char* []);

int SendMacro0_cb (int argc, char* argv[]);

int StartTest_cb (int, char* []);

int StopTest_cb (int, char* []);

int MoveRelative_cb (int, char* []);

int MoveHome_cb (int, char* []);

int ResetSmacMotor_cb (int argc, char* argv[]);

int setSmacParam_cb (int argc, char* argv[]);

int ChangeTimerFreq_cb (int argc, char* argv[]);

int setAllSmacParams_cb (int argc, char* argv[]);

int ReadAdc_cb (int argc, char* argv[]);

int ReadEncoder_cb (int argc, char* argv[]);

int ResetEncoder_cb (int argc, char* argv[]);

int TellPosition_cb (int argc, char* argv[]);

int TellTorque_cb (int argc, char* argv[]);

int CanConfigFilter_cb (int argc, char* argv[]);

int CanRemoveFilters_cb (int argc, char* argv[]);

int LinConfigFilter_cb (int argc, char* argv[]);

int LinRemoveFilters_cb (int argc, char* argv[]);

int SetPiece_cb (int argc, char* argv[]);

int SelectSmac_cb (int argc, char* argv[]);

int SelectEncoder_cb (int argc, char* argv[]);

int SelectAdc_cb (int argc, char* argv[]);

int MotorOff_cb (int argc, char* argv[]);

int MotorOn_cb (int argc, char* argv[]);

int Last_cmd_cb (int, char* []);

//int Help_cb (int, char* []);

int idle_cb (int, char* []);

#endif /* INC_KEYTEST_H_ */
