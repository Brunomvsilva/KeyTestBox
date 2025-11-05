#ifndef INC_MAX1032_H_
#define INC_MAX1032_H_

#include "stm32f767xx.h"
#include "spi.h"
#include "stdint.h"

#define TARE_ITERATION_VAL 50

typedef struct {

	SPI_HandleTypeDef *spiHandle;
	GPIO_TypeDef *csPort;
	uint16_t csPin;
	uint8_t rxBuf[4];

	uint16_t raw_value;
	//float adcValue;
	//float tare;

} s_MAX1032;


uint8_t MAX1032_Init(s_MAX1032 *adc_ptr, SPI_HandleTypeDef *spiHandle, GPIO_TypeDef *csPort, uint16_t csPin, uint8_t channel);

float ReadADC_Polling(s_MAX1032 *adc_ptr, uint8_t channel);

void ReadADCISR_Request(s_MAX1032 *adc_ptr, uint8_t channel);

#endif /* INC_MAX1032_H_ */
