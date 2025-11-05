/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tim.h"
#include "spi.h"
#include "keytest.h"
#include "MAX1032.h"
#include "fifo.h"
#include "mqtt_com.h"
#include "cmdparser.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "lwip.h"
#include "boxUserConfig.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint8_t CAN_RxData[CAN_MAX_NUM_INTEREST_IDS][8];
bool CAN_MessageProcessed[CAN_MAX_NUM_INTEREST_IDS];
CAN_RxHeaderTypeDef CAN_RxHeader[CAN_MAX_NUM_INTEREST_IDS];

#if USE_ADC_INTERRUPT
extern uint8_t SPI_Transfer_Tx[4];
extern uint8_t SPI_Transfer_Rx[5];
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

uint8_t LIN_Receive = 0;

char outputTestData[SIZE_OUTPUT_BUFFER] = "";
uint32_t RemainingOutputBytes;
uint32_t numOutputBytes;

ST_FIFO SmacRecvFIFO;

extern Command KeytestBox_cmd_list[NUM_CMDS];
uint32_t index_output = 0;

xQueueHandle MqttRequestQueue;
xQueueHandle SmacOperationsQueue;

s_KeyTest* st_Keytest;

extern uint8_t UUT_LIN_RxData[16];

extern char TestParamsSmac [NUM_PARAMS][NUM_PARAM_ELEMENTS][PARAMS_SIZE];

extern uint8_t reconnect;
extern mqtt_client_t *mqtt_client_instance;

uint32_t sampleNUM = 0;

uint8_t idlebyte;
uint8_t accept_chars_smac;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId KeyTestTaskHandle;
osThreadId MqttTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void KeytestTask(void const * argument);
void MqttTaskFunction(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  st_Keytest = (s_KeyTest*)calloc(1, sizeof(s_KeyTest));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of KeyTestTask */
  osThreadDef(KeyTestTask, KeytestTask, osPriorityRealtime, 0, 2048);
  KeyTestTaskHandle = osThreadCreate(osThread(KeyTestTask), (void*)st_Keytest);

  /* definition and creation of MqttTask */
  osThreadDef(MqttTask, MqttTaskFunction, osPriorityLow, 0, 4096);
  MqttTaskHandle = osThreadCreate(osThread(MqttTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(portMAX_DELAY);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_KeytestTask */
/**
* @brief Function implementing the KeytestHandle thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_KeytestTask */
void KeytestTask(void const * argument)
{
  /* USER CODE BEGIN KeytestTask */
  float sampleADC;
  int16_t sampleEncoder;
  uint8_t eOperation;

  //Typecast argument to receive keytest instance
  s_KeyTest* keytest_h = (s_KeyTest*) argument;


  if(xTaskGetCurrentTaskHandle() == KeyTestTaskHandle)
  {
	  KeyTestInit(keytest_h, TestParamsSmac, DEFAULT_KEYTEST_FREQ, outputTestData);
	  SmacOperationsQueue = xQueueCreate(64, sizeof(uint8_t));
	  HAL_UART_Receive_IT(&huart4,&idlebyte, 1);
	  HAL_UART_Receive_IT(&huart5,&idlebyte, 1);
  }

  memset(outputTestData, '\0', sizeof(outputTestData));

  /* Infinite loop */
  for(;;)
  {

	//WAIT TASKNOTIFICATION
	xQueueReceive(SmacOperationsQueue, &eOperation, pdMS_TO_TICKS(5));

	switch (eOperation)
	{

	case OP_TEST_SAMPLE:
	    eOperation = 0;

	#if USE_ADC_INTERRUPT
	    uint16_t aux = 0;
	    // Sample Values
	    // Value sent from piece is Read and updated outside by ISR
	   // HAL_TIM_Base_Stop(&htim14);
	    //htim14.Instance->CNT = 0;
	    //HAL_TIM_Base_Start(&htim14);

	    ReadADCISR_Request(keytest_h->p_adc, keytest_h->adcChannel);
	    sampleEncoder = ReadEncoder(keytest_h);



	    index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "%d", sampleEncoder);
	    for(int i = 0; i<st_Keytest->numInterestIDs; i++)
	    	index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, " %lX", st_Keytest->UUT_ReadValues[i]);

	    // Wait for ADC to finish reading
	    xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(5));

	    // Process ADC Value and add to output buffer
	    aux = (SPI_Transfer_Rx[2] << 8) | SPI_Transfer_Rx[3];
	    aux >>= 2;

	    // Use fixed-point arithmetic for sampleADC calculation
	    const int scale_factor = 16383;
	    const float conversion_factor = 24.576 / scale_factor;
	    sampleADC = (float)aux * conversion_factor - 12.288;

	    // Format ADC output string directly
	    index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, " %.3f\r", sampleADC);
	    //index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "%d %.8lX %.3f\r\n", sampleEncoder, sampleKey, sampleADC);
	    // Increment number of samples
	   // HAL_TIM_Base_Stop(&htim14);
	    //index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, " %ld", htim14.Instance->CNT);
	    //index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "\r");

	    // Reactivate CAN ISR
	    if (st_Keytest->UUT_Protocol == CAN_PROTOCOL)
	    {
	    	if(st_Keytest->isCanFIFO0_Active)
	      	    HAL_CAN_ActivateNotification(st_Keytest->CanPort, CAN_IT_RX_FIFO0_MSG_PENDING);

	    	if(st_Keytest->isCanFIFO1_Active)
	    		HAL_CAN_ActivateNotification(st_Keytest->CanPort, CAN_IT_RX_FIFO1_MSG_PENDING);
	    }

	    break;
	#else
	    // Sample Values
	    // Value sent from piece is Read and updated outside by ISR

	   // HAL_TIM_Base_Stop(&htim14);
	   // htim14.Instance->CNT = 0;
	    //HAL_TIM_Base_Start(&htim14);
	    sampleADC = ReadADC_Polling(keytest_h->p_adc, keytest_h->adcChannel);
	    sampleEncoder = ReadEncoder(keytest_h);
   	   // sampleKey = (int16_t)(keytest_h->UUT_ReadValue);


   	    index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "%d %.3f", sampleEncoder, sampleADC);

   	    for(int i = 0; i<st_Keytest->numInterestIDs; i++)
   	    	index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, " %lX", st_Keytest->UUT_ReadValues[i]);

   	    //index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "\r");



   	   // HAL_TIM_Base_Stop(&htim14);
   	    //index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "%ld", htim14.Instance->CNT);
   	    index_output += snprintf(outputTestData + index_output, sizeof(outputTestData) - index_output, "\r");

   	    //sampleADC = 0;
   	    //htim14.Instance->CNT = 0;

   	    // Reactivate CAN ISR
   	    if (st_Keytest->UUT_Protocol == CAN_PROTOCOL)
   	    {
   	    	if(st_Keytest->isCanFIFO0_Active)
   	    		HAL_CAN_ActivateNotification(st_Keytest->CanPort, CAN_IT_RX_FIFO0_MSG_PENDING);

   	    	if(st_Keytest->isCanFIFO1_Active)
   	    		HAL_CAN_ActivateNotification(st_Keytest->CanPort, CAN_IT_RX_FIFO1_MSG_PENDING);
   	    }
   	    break;
    #endif


		case OP_START_TEST:
			eOperation = 0;
			ClearTestTimer(keytest_h);
			sampleNUM = 0;
			memset(outputTestData, '\0', sizeof(outputTestData));

			if(keytest_h->UUT_Protocol == CAN_PROTOCOL)
			{
				//Activate CAN Notifications for when a message with target ID arrives at the corresponding FIFO
				if(st_Keytest->isCanFIFO0_Active)
					HAL_CAN_ActivateNotification(keytest_h->CanPort, CAN_IT_RX_FIFO0_MSG_PENDING);
				if(st_Keytest->isCanFIFO1_Active)
					HAL_CAN_ActivateNotification(keytest_h->CanPort, CAN_IT_RX_FIFO1_MSG_PENDING);
			}
			else
			if(st_Keytest->UUT_Protocol == LIN_PROTOCOL)
			{
				//Start Listening to LIN BUS -> ISR occurs when (UUT_NumDataBytes + Header) Bytes are received
				HAL_UART_Receive_IT(keytest_h->LinPort, UUT_LIN_RxData, keytest_h->UUT_InterestIDsInfo[0].NumDataBytes + LIN_HEADER_BYTES);
			}
			else
			{
				//If LIN/CAN are not setup and the command sent is Start Test, Warn about the Error
				if(keytest_h->enabled)
				{
					Mqtt_AddSendOperation(ERROR_TOPIC, "CAN/LIN are not setup\n");
					keytest_h->enabled = 0;
					st_Keytest->status = S_READY;
					//keytest_h->test_count = 0;
				}
				else
				{
					//The command sent was resetsmac or ms0 while a test was not running
					HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"ms0\r", strlen("ms0\r"), HAL_MAX_DELAY);
					st_Keytest->status = S_READY;
					Mqtt_AddSendOperation(LOG_TOPIC, "ms0 Sent\n");
				}
				break;
			}


			if(st_Keytest->enabled)
			{
				//The command sent was ms0 and the CAN/LIN is correctly setup
				//Run Test
				st_Keytest->status = S_BUSY;
				index_output = 0;
				HAL_UART_Transmit(keytest_h->SmacPort, (uint8_t*)"ms38\r", strlen("ms38\r"), HAL_MAX_DELAY);
			}
			else
			{
				//Command sent was stoptest or ms0 during a test
				st_Keytest->status = S_READY;
				HAL_UART_Transmit(st_Keytest->SmacPort, (uint8_t*)"ms0\r", strlen("ms0\r"), HAL_MAX_DELAY);
				Mqtt_AddSendOperation(LOG_TOPIC, "ms0 Sent\r\n");
			}
			break;


		case OP_UPDATE_CAN_VALUE:
			//CAN is being updated directly in the CAN_FIFO Interrupt but it can be updated here if you Send an operation from CAN_FIFO
			eOperation = 0;
			for(int i = 0; i < st_Keytest->numInterestIDs; i++)
			{
				if(!CAN_MessageProcessed[i])
				{
					st_Keytest->UUT_ReadValues[i] = readBits(CAN_RxData[i], keytest_h->UUT_InterestIDsInfo[i].startBit, keytest_h->UUT_InterestIDsInfo[i].numBits);
					CAN_MessageProcessed[i] = 1;
				}
			}
			//st_Keytest->UUT_ReadValue = readBits(CAN_RxData, keytest_h->.startBit, keytest_h->UUT_ReadInfo.numBits);
			break;


		case OP_UPDATE_LIN_VALUE:
			eOperation = 0;
			//Read Lin Value
			st_Keytest->UUT_ReadValues[st_Keytest->numInterestIDs - 1] = readBits(UUT_LIN_RxData,keytest_h->UUT_InterestIDsInfo[st_Keytest->numInterestIDs -1].startBit, keytest_h->UUT_InterestIDsInfo[st_Keytest->numInterestIDs -1].numBits);
			HAL_UART_Receive_IT(keytest_h->LinPort, UUT_LIN_RxData, keytest_h->UUT_InterestIDsInfo[0].NumDataBytes + LIN_HEADER_BYTES);
			break;


		case OP_SMAC_MSG:
			eOperation = 0;
			ProcessSmacMessage(keytest_h);
			break;


		default:
			eOperation = 0;
			break;
	}
  }
  /* USER CODE END KeytestTask */
}

/* USER CODE BEGIN Header_MqttTaskFunction */
/**
* @brief Function implementing the MqttTaskHandle thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_MqttTaskFunction */
void MqttTaskFunction(void const * argument)
{
  /* USER CODE BEGIN MqttTaskFunction */
	err_t mqtt_err;
	int keytestbox_err;
	BaseType_t xRet;
	MqttRequest_t MqttRequest;

	MX_LWIP_Init();

	MqttRequestQueue = xQueueCreate(16, sizeof(MqttRequest));

	Mqtt_AddConnectOperation();

//  /* Infinite loop */
  for(;;)
  {

	  xQueueReceive(MqttRequestQueue, &MqttRequest, portMAX_DELAY);

	  switch(MqttRequest.eOperation)
	  {

	  case eConnect:

		  if(!mqtt_client_is_connected(mqtt_client_instance))
		  {
			  xRet = bsp_mqtt_connect();       //Connect to broker
			  if(xRet != ERR_OK && xRet != ERR_ALREADY && xRet != ERR_ISCONN)
			  {
				  osDelay(500);
				  mqtt_disconnect(mqtt_client_instance);
				  Mqtt_AddConnectOperation();
			  }

		  }
		  else
			  Mqtt_AddSendOperation(LOG_TOPIC, "Already Connected!\r\n");

		  break;

	  case eReceive:
		  //Ver se precisa de setup
		  keytestbox_err = ucmd_parse(KeytestBox_cmd_list, UCMD_DEFAULT_DELIMETER, (const char*)MqttRequest.data);

		  if(keytestbox_err)
			  Mqtt_AddErrorHandleOperation(keytestbox_err);

		  break;

	  case eSend:
		  mqtt_err = bsp_mqtt_publish(mqtt_client_instance, MqttRequest.topic, MqttRequest.data, strlen(MqttRequest.data), 0, 0);

		  if(mqtt_err != ERR_OK)
		  {
			  Mqtt_AddErrorHandleOperation(mqtt_err);
			  //Retry Same operation, add it to queue
			  Mqtt_AddSendOperation(MqttRequest.topic, MqttRequest.data);
		  }

		  break;

	  case eOutputTestData:

		  //printf("%d\n", sampleNUM);
		  //printf("%s", outputTestData);

		  st_Keytest->status = S_OUTPUT;

		  char mqttmsg1[32] = "";
		  char mqttmsg2[32] = "";
		  int32_t testFrequency = (int32_t) st_Keytest->TestFrequency_Hz;

		  float testTime =(float)(1.0f/testFrequency);
		  testTime *= (float)sampleNUM;

		  sprintf(mqttmsg1, "Test time %.3f s\r\n", testTime);
		  sprintf(mqttmsg2, "outputting %ld samples\r\n", sampleNUM);

		  //Send Test Time Wait for publish to finish
		  bsp_mqtt_publish(mqtt_client_instance, KEYTEST1_TOPIC, mqttmsg1, strlen(mqttmsg1), 2, 0);
		  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		  //Send Output Start Message Wait for publish to finish
		  bsp_mqtt_publish(mqtt_client_instance, KEYTEST1_TOPIC, mqttmsg2, strlen(mqttmsg2), 2, 0);
		  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);


		  RemainingOutputBytes = getNumOutputBytes();
		  if (RemainingOutputBytes <= MQTT_SIZE_EACH_PUBLISH)
		  {
			  // Publish the entire array if the size is less than MQTT_SIZE_EACH_PUBLISH bytes
			  bsp_mqtt_publish(mqtt_client_instance, KEYTEST1_TOPIC, outputTestData, numOutputBytes, 2, 0);
			  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
			  index_output = 0;
			  sampleNUM = 0;
			  st_Keytest->status = S_READY;
			  break;
			  // Wait for notification, update SMAC state, and break
			  //esperar notificação, atualizar SMAC state e fazer break;
		  }
		  else
		  {
			  // If the array size exceeds MQTT_SIZE_EACH_PUBLISH bytes, publish in chunks of MQTT_SIZE_EACH_PUBLISH bytes
			  int chunkCount = (int) (RemainingOutputBytes / MQTT_SIZE_EACH_PUBLISH);
			  int currentChunk = 0;
			  while (currentChunk < chunkCount)
			  {
				  // Publish the current chunk of MQTT_SIZE_EACH_PUBLISH bytes
				  bsp_mqtt_publish(mqtt_client_instance, KEYTEST1_TOPIC, outputTestData + (currentChunk * MQTT_SIZE_EACH_PUBLISH), MQTT_SIZE_EACH_PUBLISH, 2, 0);
				  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
				  currentChunk++;
			  }
			  // Publish the remaining bytes (less than MQTT_SIZE_EACH_PUBLISH)
			  RemainingOutputBytes = RemainingOutputBytes % MQTT_SIZE_EACH_PUBLISH;
			  bsp_mqtt_publish(mqtt_client_instance, KEYTEST1_TOPIC, outputTestData + (chunkCount * MQTT_SIZE_EACH_PUBLISH), RemainingOutputBytes, 2, 0);
			  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

			  //Send "OutputFinished" Message
			  strcpy(mqttmsg1, "Output Finished\r\n");
			  bsp_mqtt_publish(mqtt_client_instance, KEYTEST1_TOPIC, mqttmsg1, strlen(mqttmsg1), 2, 0);
			  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);


			  st_Keytest->status = S_READY;
		  }

		  index_output = 0;
		  sampleNUM = 0;

		  break;

	  case eHandleMqttError:
		  Mqtt_Error_Handler(MqttRequest.error);

		  break;

	  }
	  osDelay(1);
  }
  /* USER CODE END MqttTaskFunction */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	uint8_t CAN_RxDataLocal[8];
	CAN_RxHeaderTypeDef RxHeaderLocal;
	//uint8_t operation = OP_UPDATE_CAN_VALUE;                   //Used if you want to process CAN Value in a Task and not in the Interrupt
	//BaseType_t pxHigherPriorityTaskWoken = pdFALSE;			 //Used if you want to process CAN Value in a Task and not in the Interrupt

	if(hcan == st_Keytest->CanPort)
	{

		if (HAL_CAN_GetRxMessage(st_Keytest->CanPort, CAN_RX_FIFO0, &RxHeaderLocal, CAN_RxDataLocal) != HAL_OK)
		{
			Error_Handler();
		}

		for(int i = 0; i < st_Keytest->numInterestIDs; i++)
		{

			if (((RxHeaderLocal.ExtId == st_Keytest->UUT_InterestIDsInfo[i].ID) && (RxHeaderLocal.IDE == st_Keytest->UUT_InterestIDsInfo[i].is_extendedID))
				|| ((RxHeaderLocal.StdId == st_Keytest->UUT_InterestIDsInfo[i].ID) && (RxHeaderLocal.IDE == st_Keytest->UUT_InterestIDsInfo[i].is_extendedID)))
			{
				//READ DIRECTLY IN INTERRUPT
				st_Keytest->UUT_ReadValues[i] = readBits(CAN_RxDataLocal, st_Keytest->UUT_InterestIDsInfo[i].startBit, st_Keytest->UUT_InterestIDsInfo[i].numBits);

				//Notify Keytest Task to Read CAN Value
				//memcpy(CAN_RxData + i * sizeof(uint8_t), CAN_RxDataLocal, sizeof(CAN_RxDataLocal));
				//CAN_MessageProcessed[i] = 0;
				//Notify Keytest Task to Read Message
				//xQueueSendFromISR(SmacOperationsQueue, &operation,&pxHigherPriorityTaskWoken);
				//portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
			}

		}
	}
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

	uint8_t CAN_RxDataLocal[8];
	CAN_RxHeaderTypeDef RxHeaderLocal;
	//uint8_t operation = OP_UPDATE_CAN_VALUE;					//Used if you want to process CAN Value in a Task and not in the Interrupt
	//BaseType_t pxHigherPriorityTaskWoken = pdFALSE;			//Used if you want to process CAN Value in a Task and not in the Interrupt

	if(hcan == st_Keytest->CanPort)
	{
		if (HAL_CAN_GetRxMessage(st_Keytest->CanPort, CAN_RX_FIFO1, &RxHeaderLocal, CAN_RxDataLocal) != HAL_OK)
		{
			Error_Handler();
		}

		for(int i = 0; i < st_Keytest->numInterestIDs; i++)
		{
			if (((RxHeaderLocal.ExtId == st_Keytest->UUT_InterestIDsInfo[i].ID) && (RxHeaderLocal.IDE == st_Keytest->UUT_InterestIDsInfo[i].is_extendedID))
				|| ((RxHeaderLocal.StdId == st_Keytest->UUT_InterestIDsInfo[i].ID) && (RxHeaderLocal.IDE == st_Keytest->UUT_InterestIDsInfo[i].is_extendedID)))
			{
				//READ DIRECTLY IN INTERRUPT
				st_Keytest->UUT_ReadValues[i] = readBits(CAN_RxDataLocal, st_Keytest->UUT_InterestIDsInfo[i].startBit, st_Keytest->UUT_InterestIDsInfo[i].numBits);
				//Notify Task to Read Can Value
				//memcpy(CAN_RxData + i * sizeof(uint8_t), CAN_RxDataLocal, sizeof(CAN_RxDataLocal));
				//Notify Keytest Task to Read Message
				//xQueueSendFromISR(SmacOperationsQueue, &operation,&pxHigherPriorityTaskWoken);
				//portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
			}

		}

	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t operationSMAC = OP_SMAC_MSG;
	BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	uint8_t operationLIN = OP_UPDATE_LIN_VALUE;

	if(huart == st_Keytest->SmacPort)
	{

		HAL_UART_Receive_IT(st_Keytest->SmacPort, (uint8_t*)&idlebyte, 1);

		if(idlebyte == '#')
			accept_chars_smac = 1;

		if(accept_chars_smac)
		{
			fifo_push(st_Keytest->SmacRecvFIFO, idlebyte);
			if(idlebyte == '?' || idlebyte == '*')
			{

				accept_chars_smac = 0;
				fifo_push(st_Keytest->SmacRecvFIFO, '\n');
				xQueueSendFromISR(SmacOperationsQueue, &operationSMAC,&pxHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
			}

			//It only enters into this IF after the user sent a TP or TQ command from MQTT
			//Otherwise, code should never reach here
			if(idlebyte == '>' && st_Keytest->status == S_READY)
			{
				accept_chars_smac = 0;
				//Remove promt character '>' sent by Smac
				fifo_pop_last(st_Keytest->SmacRecvFIFO);
				//After this queue Send, the Keytest task Receives a SMAC Message Operation
				//The task will verify that the Smac message is not one that normally is sent during the test
				//So it can only be a response to commands such as TP or TQ
				xQueueSendFromISR(SmacOperationsQueue, &operationSMAC,&pxHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
			}
		}
	}

	//If its a LIN Message
	if(huart == st_Keytest->LinPort)
	{
		//Read ID Byte of Lin message
		uint8_t ID_byte = UUT_LIN_RxData[2];
		//Remove Parity bits of ID
		ID_byte = ID_byte & 0x3F;
		//Verify if its an ID of interest Message
		if(ID_byte == st_Keytest->UUT_InterestIDsInfo[st_Keytest->numInterestIDs - 1].ID)
		{
			//Notify Keytest Task to Read Message
			xQueueSendToFrontFromISR(SmacOperationsQueue, &operationLIN,&pxHigherPriorityTaskWoken);
			//Make another LIN Message Request
			portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
		}

	}

}


#if USE_ADC_INTERRUPT
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

	if(hspi == st_Keytest->p_adc->spiHandle)
	{
		//Bring ADC CS Pin back up
		HAL_GPIO_WritePin(st_Keytest->p_adc->csPort, st_Keytest->p_adc->csPin, GPIO_PIN_SET);

		if(st_Keytest->status == S_BUSY)
		{
			//Notify Keytest Task that the Read ADC was complete
			xTaskNotifyFromISR(KeyTestTaskHandle, 0, 0, &pxHigherPriorityTaskWoken);
			portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
		}
	}
}
#endif


void ProcessSmacMessage(s_KeyTest* keytest_h)
{
	uint8_t esc = 0x1b;
	char msg[20] = "";


	fifo_get_message(keytest_h->SmacRecvFIFO, msg);



	if(!strcmp(msg, "#START?\n"))
	{
		if(keytest_h->enabled)
		{
			st_Keytest->status = S_BUSY;
			strcpy(keytest_h->p_output, "");
			ClearTestTimer(keytest_h);
			StartTestTimer(keytest_h);
			HAL_UART_Transmit(keytest_h->SmacPort, (uint8_t*)"\r", 1, HAL_MAX_DELAY);
		}
		else
		{
			st_Keytest->status = S_READY;
			HAL_UART_Transmit(keytest_h->SmacPort, &esc, 1, HAL_MAX_DELAY);
			StopTestTimer(keytest_h);
		}
	}
	else
	if(!strcmp(msg, "#END MEASURE*\n"))
	{
		keytest_h->enabled = 0;
		StopTestTimer(keytest_h);

		if(st_Keytest->UUT_Protocol == CAN_PROTOCOL)
		{
			if(st_Keytest->isCanFIFO0_Active)
				HAL_CAN_DeactivateNotification(st_Keytest->CanPort, CAN_IT_RX_FIFO0_MSG_PENDING);

			if(st_Keytest->isCanFIFO1_Active)
				HAL_CAN_DeactivateNotification(st_Keytest->CanPort, CAN_IT_RX_FIFO1_MSG_PENDING);
		}
		else
			HAL_UART_AbortReceive_IT(st_Keytest->LinPort);

		Mqtt_AddOutputOperation(LOG_TOPIC, keytest_h->p_output);
		//vTaskSuspend(KeyTestTaskHandle);

	}
	else
	if(!strcmp(msg, "#HOMEPOS?\n"))
		HAL_UART_Transmit(keytest_h->SmacPort, (uint8_t*)"\r", 1, HAL_MAX_DELAY);
	else
	if(!strcmp(msg, "#AT HOME*\n"))
	{
		osDelay(400);
		//HAL_TIM_Encoder_Stop(keytest_h->Encoder, TIM_CHANNEL_ALL);
		keytest_h->Encoder->Instance->CNT = 0;
		//HAL_TIM_Encoder_Start(keytest_h->Encoder, TIM_CHANNEL_ALL);
	}
	else
	if(KeyTestSetParams(keytest_h, msg))
		return;
	else
	{
		//Message is a response to a TP or TQ command
		int newMsgIndex = 0;
		char FormatedMsg[20] = "";

		for (int oldMsgIndex = 0; msg[oldMsgIndex] != '\0'; oldMsgIndex++)
		{
			if (msg[oldMsgIndex] != '\r' && msg[oldMsgIndex] != '\n')
			{
				FormatedMsg[newMsgIndex] = msg[oldMsgIndex];
				newMsgIndex++;
			}
		}

		FormatedMsg[newMsgIndex] = '\0';
		Mqtt_AddSendOperation(LOG_TOPIC, FormatedMsg);
	}

}


/* USER CODE END Application */

