#include "mqtt_com.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "lwip/mem.h"
#include "lwip.h"
#include "cmdparser.h"
#include "assert.h"
#include "keytest.h"
#include "boxUserConfig.h"


extern osThreadId MqttTaskHandle;
extern xQueueHandle MqttRequestQueue;

extern s_KeyTest* st_Keytest;

/*MQTT Client Instance*/
mqtt_client_t *mqtt_client_instance = NULL;
uint8_t connection_tries = 0;
extern uint8_t EthLinkDown;

/*MQTT Publish Mutex*/
#ifdef USE_MQTT_MUTEX
SemaphoreHandle_t s__mqtt_publish_mutex = NULL;
#endif

extern char outputTestData[SIZE_OUTPUT_BUFFER];


/*MQTT Receive Struct*/
struct mqtt_recv_buffer
{
    char recv_buffer[1024];
    uint16_t recv_len;
    uint16_t recv_total;
};

/**/
struct mqtt_recv_buffer s__mqtt_recv_buffer_g = {
    .recv_len = 0,
    .recv_total = 0,
};



/*!

*/
__weak int mqtt_rec_data_process(void* arg, char *rec_buf, uint64_t buf_len)
{

    Mqtt_AddReceiveOperation(rec_buf);

    return 0;
}


/*!

*/
static void bsp_mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
	len++;

    if( (data == NULL) || (len == 0) )
    {
        //printf("mqtt_client_incoming_data_cb: condition error @entry\n");
        return;
    }

    if(s__mqtt_recv_buffer_g.recv_len + len < sizeof(s__mqtt_recv_buffer_g.recv_buffer))
    {
        //
        snprintf(&s__mqtt_recv_buffer_g.recv_buffer[s__mqtt_recv_buffer_g.recv_len], len, "%s", data);
        s__mqtt_recv_buffer_g.recv_len += len;
    }

    if ( (flags & MQTT_DATA_FLAG_LAST) == MQTT_DATA_FLAG_LAST )
    {

        mqtt_rec_data_process(arg , s__mqtt_recv_buffer_g.recv_buffer, s__mqtt_recv_buffer_g.recv_len);


        s__mqtt_recv_buffer_g.recv_len = 0;


        memset(s__mqtt_recv_buffer_g.recv_buffer, 0, sizeof(s__mqtt_recv_buffer_g.recv_buffer));


    }

}


/*!

*/
static void bsp_mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    if( (topic == NULL) || (tot_len == 0) )
    {
        //printf("bsp_mqtt_incoming_publish_cb: condition error @entry\n");
        return;
    }

	//printf("bsp_mqtt_incoming_publish_cb: topic = %s.\n",topic);
	//printf("bsp_mqtt_incoming_publish_cb: tot_len = %ld.\n",tot_len);
	s__mqtt_recv_buffer_g.recv_total = tot_len;
	s__mqtt_recv_buffer_g.recv_len = 0;

    memset(s__mqtt_recv_buffer_g.recv_buffer, 0, sizeof(s__mqtt_recv_buffer_g.recv_buffer));
}





/*!
Connection Callback
*/
static void bsp_mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{

    if( client == NULL )
    {
        //printf("bsp_mqtt_connection_cb: condition error entry\n");
    	if(!EthLinkDown)
    		Mqtt_AddConnectOperation();
        return;
    }
    if(status == MQTT_CONNECT_DISCONNECTED || status == MQTT_CONNECT_TIMEOUT)
    {
    	if(!EthLinkDown)
    		Mqtt_AddConnectOperation();
    	return;
    }

    if ( status == MQTT_CONNECT_ACCEPTED )
    {
    	EthLinkDown = 0;

    	connection_tries = 0;

    	Mqtt_AddSendOperation(LOG_TOPIC, "KEYTESTBOX Connected!");

		mqtt_set_inpub_callback(client, bsp_mqtt_incoming_publish_cb, bsp_mqtt_incoming_data_cb, arg);


		bsp_mqtt_subscribe(client,COMMAND_TOPIC,0);

		//bsp_mqtt_publish(mqtt_client_instance, SUBTOPIC, out,sizeof(out),1,0);


    }
	else
	{
		//printf("bsp_mqtt_connection_cb: Fail connected, status = %s\n", lwip_strerr(status) );


	}
}
/*!

*/
err_t bsp_mqtt_connect(void)
{
   // printf("bsp_mqtt_connect: Enter!\n");
	err_t ret;





	struct mqtt_connect_client_info_t  mqtt_connect_info = {
			"MQTT_Test",
			NULL,
			NULL,
			0,
			"/public/TEST/pub",
			"Offline_pls_check",
			0,
			0
	};

    ip_addr_t server_ip;
    ip4_addr_set_u32(&server_ip, ipaddr_addr(MQTT_HOST_IP_ADDRESS));

    uint16_t server_port = 1883;

    if (mqtt_client_instance == NULL)
    {
	    mqtt_client_instance = mqtt_client_new();
    }

	if (mqtt_client_instance == NULL)
	{
	//	printf("bsp_mqtt_connect: mqtt_client_instance malloc fail\n");
		return ERR_MEM;
	}


	LWIP_ASSERT("Number of Connection Tries Exceeded (7)", connection_tries < 7);
	connection_tries++;

	ret = mqtt_client_connect(mqtt_client_instance, &server_ip, server_port, bsp_mqtt_connection_cb, NULL, &mqtt_connect_info);





 //   printf("bsp_mqtt_connect: connect to mqtt %s\n", lwip_strerr(ret));

	return ret;
}


/*

*/
static void mqtt_client_pub_request_cb(void *arg, err_t result)
{

    //mqtt_client_t *client = (mqtt_client_t *)arg;
    if (result != ERR_OK)
    {
        //printf("mqtt_client_pub_request_cb: c002: Publish FAIL, result = %s\n", lwip_strerr(result));


    }
	else
	{
        //printf("mqtt_client_pub_request_cb: c005: Publish complete!\n");
		if(st_Keytest->status == S_OUTPUT)
			xTaskGenericNotify(MqttTaskHandle, 0, 0, NULL);
	}
}



/*!

*/
err_t bsp_mqtt_publish(mqtt_client_t *client, char *pub_topic, char *pub_buf, uint16_t data_len, uint8_t qos, uint8_t retain)
{
	if ( (client == NULL) || (pub_topic == NULL) || (pub_buf == NULL) || (data_len == 0) || (qos > 2) || (retain > 1) )
	{
		//printf("bsp_mqtt_publish: input error\n" );
		return ERR_VAL;
	}

    if(mqtt_client_is_connected(client) != pdTRUE)
    {
		//printf("bsp_mqtt_publish: client is not connected\n");
        return ERR_CONN;
    }

	err_t err;
#ifdef USE_MQTT_MUTEX

    if (s__mqtt_publish_mutex == NULL)
    {
		//printf("bsp_mqtt_publish: create mqtt mutex ! \n" );
        s__mqtt_publish_mutex = xSemaphoreCreateMutex();
    }

    if (xSemaphoreTake(s__mqtt_publish_mutex, portMAX_DELAY) == pdPASS)
#endif /* USE_MQTT_MUTEX */

    {
	    err = mqtt_publish(client, pub_topic, pub_buf, data_len, qos, retain, mqtt_client_pub_request_cb, (void*)client);
	    //printf("bsp_mqtt_publish: mqtt_publish err = %s\n", lwip_strerr(err) );

#ifdef USE_MQTT_MUTEX
        //printf("bsp_mqtt_publish: mqtt_publish xSemaphoreTake\n");
        xSemaphoreGive(s__mqtt_publish_mutex);
#endif /* USE_MQTT_MUTEX */

    }
	return err;
}







/*!

*/
static void bsp_mqtt_request_cb(void *arg, err_t err)
{
    if ( arg == NULL )
    {
        //printf("bsp_mqtt_request_cb: input error@@\n");
        return;
    }

   // mqtt_client_t *client = (mqtt_client_t *)arg;

    if ( err != ERR_OK )
    {
        //printf("bsp_mqtt_request_cb: FAIL sub, sub again, err = %s\n", lwip_strerr(err));


    }
	else
	{
		//printf("bsp_mqtt_request_cb: sub SUCCESS!\n");
	}
}

/*!

*/
err_t bsp_mqtt_subscribe(mqtt_client_t* mqtt_client, char * sub_topic, uint8_t qos)
{
    //printf("bsp_mqtt_subscribe: Enter\n");

	if( ( mqtt_client == NULL) || ( sub_topic == NULL) || ( qos > 2 ) )
	{
        //printf("bsp_mqtt_subscribe: input error@@\n");
		return ERR_VAL;
	}

	if ( mqtt_client_is_connected(mqtt_client) != pdTRUE )
	{
		//printf("bsp_mqtt_subscribe: mqtt is not connected, return ERR_CLSD.\n");
		return ERR_CLSD;
	}

	err_t err;
	err = mqtt_subscribe(mqtt_client, sub_topic, qos, bsp_mqtt_request_cb, (void *)mqtt_client);  // subscribe and call back.

	if (err != ERR_OK)
	{
		//printf("bsp_mqtt_subscribe: mqtt_subscribe Fail, return:%s \n", lwip_strerr(err));
	}
	else
	{
		//printf("bsp_mqtt_subscribe: mqtt_subscribe SUCCESS, reason: %s\n", lwip_strerr(err));
	}

	return err;
}



BaseType_t Mqtt_AddConnectOperation()
{
	MqttRequest_t xRequest;

	xRequest.eOperation = eConnect;

	xQueueSend(MqttRequestQueue, &xRequest, portMAX_DELAY);

	return 1;

}

BaseType_t Mqtt_AddSendOperation (char topic[], char data[])
{
	MqttRequest_t xRequest;
	//BaseType_t xret;


	xRequest.eOperation = eSend;
	strcpy(xRequest.data, data);
	strcpy(xRequest.topic, topic);


	xQueueSend(MqttRequestQueue, &xRequest, portMAX_DELAY);  //Send Operation to the Queue

	return 1;

}

BaseType_t Mqtt_AddOutputOperation(char topic[], char* p_data)
{
	MqttRequest_t xRequest;
	//BaseType_t xret;


	xRequest.eOperation = eOutputTestData;
	xRequest.p_out = p_data;
	strcpy(xRequest.topic, topic);


	xQueueSend(MqttRequestQueue, &xRequest, portMAX_DELAY);  //Send Operation to the Queue

	return 1;
}

BaseType_t Mqtt_AddReceiveOperation(char data[])
{
	MqttRequest_t xRequest;


	xRequest.eOperation = eReceive;
	strcpy(xRequest.data, data);

	xQueueSend(MqttRequestQueue, &xRequest, portMAX_DELAY);  //Send Operation to the Queue

	return 1;

}

BaseType_t Mqtt_AddErrorHandleOperation(int error)
{
	MqttRequest_t ErrorHandleRequest;

	ErrorHandleRequest.eOperation = eHandleMqttError;
	ErrorHandleRequest.error = error;

	xQueueSend(MqttRequestQueue, &ErrorHandleRequest, portMAX_DELAY);

	return 1;
}

void Mqtt_Error_Handler(int error)
{
	switch (error)
	{
		case UCMD_CMD_LIST_NOT_FOUND:
			Mqtt_AddSendOperation(ERROR_TOPIC, "Command List not found\r\n");
			break;
		case UCMD_CMD_NOT_FOUND:
			Mqtt_AddSendOperation(ERROR_TOPIC, "Command not found\r\n");
			break;
		case UCMD_ARGS_NOT_VALID:
			Mqtt_AddSendOperation(ERROR_TOPIC, "Bad arguments for last CMD\r\n");
			break;
		case UCMD_NUM_ARGS_NOT_VALID:
			Mqtt_AddSendOperation(ERROR_TOPIC, "Invalid number of arguments for last CMD\r\n");
			break;
		case UCMD_CMD_LAST_CMD_LOOP:
			Mqtt_AddSendOperation(ERROR_TOPIC, "Command unavailable\r\n");
			break;
		case ERR_CONN:
			Mqtt_AddConnectOperation();
			break;
		case ERR_MEM:
			//print debugger "ERR_MEM ERROR"
			configASSERT(0);  //Out of MEM on MQTT/LWIP
			break;
		default:
			//print debugger "Unhandled ERROR"
			configASSERT(0);  //Error not Handled, Check ERROR CODE
			break;
	}
}

