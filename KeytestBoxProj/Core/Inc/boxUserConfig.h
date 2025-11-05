#ifndef INC_BOXUSERCOFIG_H_
#define INC_BOXUSERCOFIG_H_

// This file sets up the Default Configs for the KeytestBox


//DEFAULT TEST FREQUENCY
//1250 Hz --> 800us per Sample
#define DEFAULT_KEYTEST_FREQ 1250


//Default frequency in MHz of the keytest timer (Timer 13), defined at CubeMX (configuration tool)
//If timer frequency changes in CubeMX, it should be changed here too
#define TIMER_CLK_FREQUENCY_MHZ 108


//Define if ADC is read by polling(Blocking mode) or Interrupt (non blocking mode)
//1 for non Blocking mode; 0 for Blocking mode
//Does not influence the performance significantly
#define USE_ADC_INTERRUPT 0

#define CAN_MAX_NUM_INTEREST_IDS 5
#define LIN_MAX_NUM_INTEREST_IDS 1

//SMAC DEFAULT TEST PARAMETERS
//These are stored when PCB resets in a array in RAM "TestParamsSmac" defined at keytest.c file
#define SMAC_DEFAULT_RPOS "0\r"
#define SMAC_DEFAULT_ACCELLERATION "20000\r"
#define SMAC_DEFAULT_VELOCITY "3000000\r"
#define SMAC_DEFAULT_TESTFORCE "32000\r"
#define SMAC_DEFAULT_MAXFORCE_FWD "15000\r"
#define SMAC_DEFAULT_TEST_INCREMENT "1\r"
#define SMAC_DEFAULT_WAIT_INCREMENT "1\r"
#define SMAC_DEFAULT_FST "1\r"
#define SMAC_DEFAULT_DIRECTION "1\r" //1 Push; 2 Pull
#define SMAC_DEFAULT_MAXFORCE_RWD "-15000\r"




//The KEYTESTBOX IP is defined in CubeMX project generator
//The Host IP that holds the MQTT Broker is defined here
#define MQTT_HOST_IP_ADDRESS "192.168.1.100"



//MQTT TOPICS
#define COMMAND_TOPIC  	 "KEYTESTBOX/CMD"	//Topic for Sending commands from Host to KeytestBox
#define ERROR_TOPIC      "KEYTESTBOX/ERROR"	//Topic for Error info from KeytestBox to Host
#define LOG_TOPIC        "KEYTESTBOX/LOG"   //Topic for Logging info from KeytestBox to Host
#define KEYTEST1_TOPIC   "KEYTESTBOX/TEST1" //Topic to where results are sent from KeytestBox to Host


//defines size of the array of characters where samples are stored during the Test
//The higher the test frequency, the higher this value should be
//If the value is too small to store all samples of the test, the execution will stop during a test due to a memory Hardfault
#define SIZE_OUTPUT_BUFFER 270000 //Bytes

//defines the size of each publish when outputting test results
//Publishes are sent in Chunks of 40000 Bytes
//must be less than MQTT_OUTPUT_RINGBUF_SIZE which is defined in lwipopts.h
//MQTT_OUTPUT_RINGBUF_SIZE > (MQTT_SIZE_EACH_PUBLISH + Mqtt packet headers)
#define MQTT_SIZE_EACH_PUBLISH 48000


#endif /* INC_BOXUSERCOFIG_H_ */
