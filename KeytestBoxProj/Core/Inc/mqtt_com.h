#ifndef __MQTT_COM_H
#define __MQTT_COM_H

#include <string.h>
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "semphr.h"

/* lwip includes. */
#include "lwip/apps/mqtt.h"
#include "lwip/ip4_addr.h"

//#define USE_MQTT_MUTEX 0

typedef enum MqttOperation
{
	eSend,                   //Send Data to Broker (Small Publishes)
	eReceive,				 //Receive Data from Broker (Commands from User)
	eConnect,				 //Connect to Broker
	eOutputTestData,		 //Output Test Data (Long Publishes)
	eHandleMqttError		 //Handle Error Operation
} Mqtt_Operation_t;

typedef struct MqttRequest
{
	Mqtt_Operation_t eOperation;    // Operation to Perform (Read or Write)
	char topic[32];				    // Mqtt topic, used when publishing data
	char data[64];					// Used When Publishing Data and When Receiving Commands from subscribed topics
	char* p_out;					//Used for publishing Keytest data
	int error;					    //Used for Error Handle Operation
} MqttRequest_t;

BaseType_t Mqtt_AddConnectOperation();

BaseType_t Mqtt_AddSendOperation(char topic[], char data[]);

BaseType_t Mqtt_AddOutputOperation(char topic[], char* p_data);

BaseType_t Mqtt_AddReceiveOperation(char data[]);

BaseType_t Mqtt_AddErrorHandleOperation(int error);

err_t bsp_mqtt_subscribe(mqtt_client_t* mqtt_client, char * sub_topic, uint8_t qos);

err_t bsp_mqtt_publish(mqtt_client_t *client, char *pub_topic, char *pub_buf, uint16_t data_len, uint8_t qos, uint8_t retain);

void bsp_mqtt_init(void);

err_t bsp_mqtt_connect(void);

void Mqtt_Error_Handler(int error);





#endif
