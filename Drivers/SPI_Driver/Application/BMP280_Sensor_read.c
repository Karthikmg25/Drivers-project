
#include <stdint.h>
#include "stm32f401re_spi_driver.h"
#include "stm32f401re_gpio_driver.h"
#include "SPI_Baremetal.h"
#include "USART_Baremetal.h"
#include "TIMERs_Baremetal.h"

// helper functions for slave selection
static void SelectSlave()
{
	// reset PB6 for slave select
	GPIOB->ODR &=~ (1<< 6);
}
static void DeselectSlave()
{
	GPIOB->ODR |= (1<< 6);
}

int App1_main(void)
{
	USART_Configuration();
	Delay_Creation();
	// Set GPIO pins as SPI
	// - SPI1_MOSI -> PA7 (D11)
	// - SPI1_MISO -> PA6 (D12)
	// - SPI1_SCK  -> PA5 (D13)
	// - SPI1_SS   -> PB6 (D10)

	SPI_GPIO_Configurations();

	SPI_Handle_t spi1;
	spi1.pSPIx = SPI1;
	spi1.SPI_Config.SPI_Bus_Config= SPI_BUS_CONFIG_FD;
	spi1.SPI_Config.SPI_Device_Mode = SPI_DEVICE_MODE_MASTER;
	spi1.SPI_Config.SPI_CLK_Speed = SPI_PRESCALAR_16;
	spi1.SPI_Config.SPI_SSM = SPI_SSM_EN;
	spi1.SPI_Config.SPI_DataFrame = SPI_DFF_8BITS;

	SPI_Init(&spi1);


    // 1) Reading Sensor ID
    //****************************************************************************************************
	// Reading sensor id of BMP280 : 0x58
	// Stored at register address : 0xD0

	SelectSlave();

	// send register address
	SPI_TransmitByte(SPI1, 0xD0);

	// receive sensor id
	uint8_t sensor_id = SPI_ReceiveByte(SPI1);

	DeselectSlave();

	// 2) Read 3 calibration values for temperature calculation
	//********************************************************************************************************************
	//    - These are 16 bit values (dig_t1,dig_t2,dig_t3)
	//    - Stored in little endian format at  register address: 0x88-0x8D (6 bytes)
	//    - digt2 and digt3 are signed values

	// Allocate a buffer for reading calibration constants
	uint8_t calib_values[6]={};

	SelectSlave();
	// send register address
	SPI_TransmitByte(SPI1, 0x88);
	SPI_Receive_Buffer(SPI1, calib_values, 6);
	DeselectSlave();

	// Convert bytes to 16 bit values
	uint16_t dig_t1 = ((uint16_t)calib_values[1]<< 8)| (((uint16_t)calib_values[0]));
	int16_t dig_t2 = ((int16_t)calib_values[3]<< 8)| (((int16_t)calib_values[2]));
	int16_t dig_t3 = ((int16_t)calib_values[5]<< 8)| (((int16_t)calib_values[4]));


	// 3) Configure control meas register
	//*********************************************************************************************************************
	// register address: 0xF4
	// Write 0x27 for setting configuration

	SelectSlave();
	// send register address
	SPI_TransmitByte(SPI1, 0xF4);
	SPI_TransmitByte(SPI1, 0x27);
	DeselectSlave();

	// 4) Read 20-bit raw ADC value
	//********************************************************************************************************************
	// register address: 0xFA-0xF
	// combine MSB,LSB,XLSB to form 20 bit value
	// ADC value= (MSB<<12)|(LSB<<8)|(XLSB>>4)
	// Read ADC values continuously in loop

	while(1)
	{
		// calculate ADC_value:

		uint8_t ADC_data[3];
		SelectSlave();
		// send register address
		SPI_TransmitByte(SPI1, 0xFA);
		SPI_Receive_Buffer(SPI1, ADC_data, 3);
		DeselectSlave();
		uint32_t ADC_value= ((uint32_t)ADC_data[0]<<12)|((uint32_t)ADC_data[1]<<4)|((uint32_t)ADC_data[2]>>4);


	   USART_Send_String("\n\n\nSensor ID: ");
	   USART_SendHex_Number(sensor_id);

	   USART_Send_String("\ndig_t1 : ");
	   USART_Send_Number(dig_t1);

	   USART_Send_String("\ndig_t2 : ");
	   USART_Send_Number(dig_t2);

	   USART_Send_String("\ndig_t3 : ");
	   USART_Send_Number(dig_t3);

	   USART_Send_String("\nADC value : ");
	   USART_Send32bit_Number(ADC_value);

	   // calculate temperature:

	   	int32_t var1 = (((((int32_t)ADC_value>> 3) - ((int32_t)dig_t1 << 1))) * ((int32_t)dig_t2)) >> 11;
	    int32_t var2 = ((((((int32_t)ADC_value >> 4) - (int32_t)dig_t1) * (((int32_t)ADC_value >> 4) - (int32_t)dig_t1)) >> 12) *(int32_t)dig_t3) >> 14;
	   	int32_t t_fine = var1 + var2;
	   	int32_t T = (t_fine * 5 + 128) >> 8;

	   	USART_Send_String("\nTemperature : ");
	    USART_Send32bit_Number(T / 100);
	    USART_Transmission('.');
	    USART_Send32bit_Number(T % 100);

		delay_ms(500);
	}


}








