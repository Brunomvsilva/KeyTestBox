#include "MAX1032.h"
#include "usart.h"
#include "stdint.h"
#include "boxUserConfig.h"


uint8_t SPI_Transfer_Tx[4] = {};
uint8_t SPI_Transfer_Rx[5] = {};

uint8_t MAX1032_Init(s_MAX1032 *adc_ptr, SPI_HandleTypeDef *spiHandle, GPIO_TypeDef *csPort, uint16_t csPin, uint8_t channel)
{
	uint8_t aux = 0;

	/* Store interface parameters in struct */
	adc_ptr->spiHandle 		= spiHandle;
	adc_ptr->csPort			= csPort;
	adc_ptr->csPin 			= csPin;

	//polling implementation
	//uint8_t Tx_AnalogInputConfig = 0b10000110; // Single-ended channel 0  (0V to 12 V)

	//uint8_t Tx_AnalogInputConfig = 0b10001001;  //Differential CH0+ CH1-    (-6V to 6V)

	//If channel = 0 -> aux = 0000
	//If channel = 1 -> aux = 0010
	//If channel = 2 -> aux = 0100
	aux = channel << 1;

	//Put channel selection in highest Nibble and activate Start bit
	aux = (aux << 4) | 0x80;

	uint8_t Tx_AnalogInputConfig = 0b00001100 | aux;  //(Diferential -12,288 V  to  12,288 V)

	uint8_t Tx_ModeControl = 0b10001000;  //External Clock mode 0


	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_RESET);

	HAL_SPI_Transmit(adc_ptr->spiHandle, &Tx_AnalogInputConfig, 1, 1000); // Analog Input Config Byte

	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_RESET);

	HAL_SPI_Transmit(adc_ptr->spiHandle, &Tx_ModeControl, 1, 1000);  	//Mode Control Byte

	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_SET);



	return 1;
}


float ReadADC_Polling(s_MAX1032 *adc_ptr, uint8_t channel)
{
	uint8_t Rx[5];
	uint16_t aux = 0;
	float result;

	uint8_t Tx_StartConversion[4] = {(channel << 5 | 0x80), 0x00, 0x00, 0x00};
	//uint8_t Tx_StartConversion[4] = {0x80, 0x00, 0x00, 0x00};

	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_RESET);

	HAL_SPI_TransmitReceive(adc_ptr->spiHandle, Tx_StartConversion, Rx , 4, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_SET);

	aux = (Rx[2] << 8) | Rx[3];
	aux >>= 2;


	const int scale_factor = 16383;
	const float conversion_factor = 24.576 / scale_factor;
    result = (float)aux * conversion_factor - 12.288;

    return result;
	//adc_ptr->adcValue = (((float)aux / 16383.0) * 24.576) - 12.288; // DIFERENTIAL BIPOLAR; REF = 4.096 V
	//adc_ptr->adcValue = (((float)aux / 16383.0) * 23.4) - 11.7  - adc_ptr->tare; //COM TARE; REF = 3.9 V
	//return (((float)aux / 16383.0) * 23.4) - 11.7; // SEM TARE; REF = 3.9 V


	//adc_ptr->adcValue = ((float)aux/16383.0) * (4.096 * 3); //SINGLE ENDED
	//HAL_UART_Transmit(&huart3, adcval, , Timeout)

	//return 1;

}

void ReadADCISR_Request(s_MAX1032 *adc_ptr, uint8_t channel)
{
	SPI_Transfer_Tx[0] = (channel << 5 | 0x80);
	SPI_Transfer_Tx[1] = 0;
	SPI_Transfer_Tx[2] = 0;
	SPI_Transfer_Tx[3] = 0;

	SPI_Transfer_Rx[0] = 0;
	SPI_Transfer_Rx[1] = 0;
	SPI_Transfer_Rx[2] = 0;
	SPI_Transfer_Rx[3] = 0;
	SPI_Transfer_Rx[4] = 0;

	//Bring CS Down
	HAL_GPIO_WritePin(adc_ptr->csPort, adc_ptr->csPin, GPIO_PIN_RESET);
	//Start SPI Transfer
	HAL_SPI_TransmitReceive_IT(adc_ptr->spiHandle, SPI_Transfer_Tx, SPI_Transfer_Rx, 4);

}


