/*
 * stm32f446xx_i2c_driver.c
 * I2C Driver for STM32F446xx MCU
 * Author: Neeraj
 */

#include <stm32f401xx_i2c_driver.h>

#include "stm32f401xx_rcc_driver.h"


static void  I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx);
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle);

static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle );
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle );

static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx)
{
	pI2Cx->CR1 |= ( 1 << I2C_CR1_START);
}



static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
	// Sending slave address for write operation
	SlaveAddr = SlaveAddr << 1;
	SlaveAddr &= ~(1); //SlaveAddr is Slave address + r/nw bit=0
	pI2Cx->DR = SlaveAddr;
}


static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
	// Sending slave address for read operation
	SlaveAddr = SlaveAddr << 1;
	SlaveAddr |= 1; //SlaveAddr is Slave address + r/nw bit=1
	pI2Cx->DR = SlaveAddr;
}


static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle )
{
	uint32_t dummy_read;
	//check for device mode
	if(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_MSL))
	{
		//device is in master mode
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			if(pI2CHandle->RxSize  == 1)
			{
				// For reception in master mode,
				// last byte is received with No Acknowledgment

				//first disable the ack
				I2C_ManageAcking(pI2CHandle->pI2Cx,DISABLE);

				//clear the ADDR flag ( read SR1 , read SR2)
				dummy_read = pI2CHandle->pI2Cx->SR1;
				dummy_read = pI2CHandle->pI2Cx->SR2;
				(void)dummy_read;
			}

		}
		else
		{
			// Initial bytes are received with ACK in multi-bytes reception

			//clear the ADDR flag ( read SR1 , read SR2)
			dummy_read = pI2CHandle->pI2Cx->SR1;
			dummy_read = pI2CHandle->pI2Cx->SR2;
			(void)dummy_read;

		}

	}
	else
	{
		//device is in slave mode
		//clear the ADDR flag ( read SR1 , read SR2)
		dummy_read = pI2CHandle->pI2Cx->SR1;
		dummy_read = pI2CHandle->pI2Cx->SR2;
		(void)dummy_read;
	}


}


 void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx)
{
	 // This releases control over the I2C bus
	pI2Cx->CR1 |= ( 1 << I2C_CR1_STOP);
}


 void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx,uint8_t EnorDi)
 {
	 // This function enables or disables the event and error interrupts.
	 if(EnorDi == ENABLE)
	 {
			pI2Cx->CR2 |= ( 1 << I2C_CR2_ITEVTEN);
			pI2Cx->CR2 |= ( 1 << I2C_CR2_ITBUFEN);
			pI2Cx->CR2 |= ( 1 << I2C_CR2_ITERREN);
	 }else
	 {
			pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVTEN);
			pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);
			pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITERREN);
	 }

 }

/*********************************************************************
 * @fn      		  - I2C_PeripheralControl
 *
 * @brief             - This function enables or disables the I2C peripheral.
 *
 * @param[in]         - pI2Cx - pointer to I2C register definition structure
 * @param[in]         - EnorDi- Enable or Disable
 *
 * @return            - none
 *
 * @Note              - Internally configures PE bit in CR1 register for peripheral control
 *
 * @Abstraction       - Provides abstraction by providing a clean API for controlling
 *                      the peripheral without directly accessing registers.
 *
 */
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
	if(EnOrDi == ENABLE)
	{
		pI2Cx->CR1 |= (1 << I2C_CR1_PE);
		//pI2cBaseAddress->CR1 |= I2C_CR1_PE_Bit_Mask;
	}else
	{
		pI2Cx->CR1 &= ~(1 << 0);
	}

}


/*********************************************************************
 * @fn      		  - I2C_PeriClockControl
 *
 * @brief             - This function enables or disables the peripheral clock for given peripheral
 *
 * @param[in]         - pI2Cx  - pointer to I2C register definition structure
 * @param[in]         - EnorDi - Enable or Disable
 *
 * @return            - none
 *
 * @Note              - Internally configures control bits in APB1EN register for peripheral clock control
 *
 * @Abstraction       - This API abstracts the clock enable registers in RCC peripheral from the application layer.
 *                      Internally uses APB1ENR rgister and configures corresponding bits.
 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pI2Cx == I2C1)
		{
			// Enables I2C1 clock by setting bit 21 in APB1ENR
			I2C1_PCLK_EN();
		}else if (pI2Cx == I2C2)
		{
			// Enables I2C2 clock by setting bit 22 in APB1ENR
			I2C2_PCLK_EN();
		}else if (pI2Cx == I2C3)
		{
			// Enables I2C3 clock by setting bit 23 in APB1ENR
			I2C3_PCLK_EN();
		}
	}
	else
	{
		if(pI2Cx == I2C1)
		{
			// Disables I2C1 clock by resetting bit 21 in APB1ENR
			I2C1_PCLK_DI();
		}else if (pI2Cx == I2C2)
		{
			// Disables I2C2 clock by resetting bit 22 in APB1ENR
			I2C2_PCLK_DI();
		}else if (pI2Cx == I2C3)
		{
			// Disables I2C3 clock by resetting bit 22 in APB1ENR
			I2C3_PCLK_DI();
		}
	}
}




/*********************************************************************
 * @fn      		  - I2C_Init
 *
 * @brief             - This function initializes the given I2C peripheral
 *
 * @param[in]         - pI2CHandle - pointer to I2C handle structure.
 *
 * @return            - none
 *
 * @Note              -

 */
void I2C_Init(I2C_Handle_t *pI2CHandle)
{
	uint32_t tempreg = 0 ;

	//enable the clock for the i2cx peripheral
	I2C_PeriClockControl(pI2CHandle->pI2Cx,ENABLE);

	//ack control bit
	//this stores the default ACK state of the peripheral.
	tempreg |= pI2CHandle->I2C_Config.I2C_AckControl << 10;
	pI2CHandle->pI2Cx->CR1 = tempreg;

	//configure the FREQ field of CR2
	//this field must be assigned with peripheral clock frequency (eg: 16 if f_PCLK = 16MHz)
	tempreg = 0;
	tempreg |= RCC_GetPCLK1Value() /1000000U ;
	pI2CHandle->pI2Cx->CR2 =  (tempreg & 0x3F);

   //program the device own address
	tempreg = 0;
	tempreg |= pI2CHandle->I2C_Config.I2C_DeviceAddress << 1;
	tempreg |= ( 1 << 14);
	pI2CHandle->pI2Cx->OAR1 = tempreg;

	//CCR calculations
	// - CCR[11:0] bits determine the SCL speed
	// - In standard mode,
	//        CCR = F_PCLK / (2 * F_SCL)
	// - In fast mode,
	//     * for 2:1 duty cycle  ->  CCR = F_PCLK / (3 * F_SCL)
	//     * for 16:9 duty cycle ->  CCR = F_PCLK / (25 * F_SCL)

	uint16_t ccr_value = 0;
	tempreg = 0;
	if(pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
	{
		//mode is standard mode
		ccr_value = (RCC_GetPCLK1Value() / ( 2 * pI2CHandle->I2C_Config.I2C_SCLSpeed ) );
		tempreg |= (ccr_value & 0xFFF);
	}else
	{
		//mode is fast mode
		tempreg |= ( 1 << 15);
		tempreg |= (pI2CHandle->I2C_Config.I2C_FMDutyCycle << 14);
		if(pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2)
		{
			ccr_value = (RCC_GetPCLK1Value() / ( 3 * pI2CHandle->I2C_Config.I2C_SCLSpeed ) );
		}else
		{
			ccr_value = (RCC_GetPCLK1Value() / ( 25 * pI2CHandle->I2C_Config.I2C_SCLSpeed ) );
		}
		tempreg |= (ccr_value & 0xFFF);
	}
	pI2CHandle->pI2Cx->CCR = tempreg;

	// TRISE[5:0] determine the maximum allowed rise time
	// - TRISE = ( F_PCLK * maximum rise time ) + 1
	// - In standard mode:
	//                     maximum rise time = 1000 ns
	// - In fast mode:
	//                     maximum rise time = 300 ns

	//TRISE Configuration
	if(pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
	{
		//mode is standard mode

		tempreg = (RCC_GetPCLK1Value() /1000000U) + 1 ;

	}else
	{
		//mode is fast mode
		tempreg = ( (RCC_GetPCLK1Value() * 300) / 1000000000U ) + 1;

	}

	pI2CHandle->pI2Cx->TRISE = (tempreg & 0x3F);

}


/*********************************************************************
 * @fn      		  - I2C_DeInit
 *
 * @brief             - This function resets the given I2C peripheral to its default state
 *
 * @param[in]         - pI2Cx - pointer to I2C register definition structure
 *
 * @return            - none
 *
 * @Note              - This function uses the RCC reset registers to reset
 *                      the I2C peripheral. After reset, all I2C registers
 *                      return to their default values.
 *
 * @Abstraction       - This API resets I2C peripheral using appropriate reset register and bits in the RCC peripheral
 *                      it hides the reset mechanism from application layer.
 *
 *
 */
void I2C_DeInit(I2C_RegDef_t *pI2Cx)
{
	// Reset mechansim:
		//
		// I2C peripheral is reset using: APB1RSTR
		//
		// Reset sequence:
		// 1. Set the corresponding bit in register.
		//    This forces the peripheral into its reset state.
		// 2. Clear the bit.
		//    This releases the peripheral from reset.
		//
		// After reset, all I2C registers return to their default values.

	if(pI2Cx == I2C1)
	{
		I2C1_REG_RESET();
	}
	else if(pI2Cx == I2C2)
	{
		I2C2_REG_RESET();
	}
	else if(pI2Cx == I2C3)
	{
		I2C3_REG_RESET();
	}


}
/*********************************************************************
 * @fn      		  - I2C_GetFlagStatus
 *
 * @brief             - This function is used to get the status of flags
 *
 * @param[in]         - pI2Cx   : pointer to I2C register definition structure
 * @param[in]         - FlagName: name of status flag
 *
 * @return            - status of flag (1 or 0)
 *
 * @Note              - none
 *
 * @Abstraction       - This API internally reads the
 *                      corresponding bit in the I2C status register (SR1),
 *                      hiding direct register access from the user.
 *
 */

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx , uint32_t FlagName)
{
	if(pI2Cx->SR1 & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}


/*********************************************************************
 * @fn      		  - I2C_SendData
 *
 * @brief             - This function is used to transmit a complete data buffer
 *
 * @param[in]         - pI2CHandle : pointer to I2C handle structure.
 * @param[in]         - pTxBuffer  : pointer to transmit buffer
 * @param[in]         - Len        : length of data to be transmitted
 * @param[in]         - SlaveAddr  : 7 bit address of slave device
 * @param[in]         - Sr         : repeated START control ( generate STOP / keep bus busy for a RESTART)
 *
 * @return            - none
 *
 * @Note              - This is blocking call. It internally uses
 *                      SB, ADDR, TXE, RXNE, BTF status flags
 *                      and Data register for transmitting each byte.
 *
 * @Abstraction       - Provides a single API to transmit data buffer over I2C.
 *                      Application doesn't need to handle status flags or data register for transmitting data.
 *
 */
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle,uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr,uint8_t Sr)
{
	// 1. Generate the START condition
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	//2. confirm that start generation is completed by checking the SB flag in the SR1
	//   Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	while( !  I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_SB)   );

	//3. Send the address of the slave with r/nw bit set to w(0) (total 8 bits )
	I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx,SlaveAddr);

	//4. Confirm that address phase is completed by checking the ADDR flag in teh SR1
	while( !  I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_ADDR)   );

	//5. clear the ADDR flag according to its software sequence
	//   Note: Until ADDR is cleared SCL will be stretched (pulled to LOW)
	I2C_ClearADDRFlag(pI2CHandle);

	//6. send the data until len becomes 0

	while(Len > 0)
	{
		while(! I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_TXE) ); //Wait till TXE is set
		pI2CHandle->pI2Cx->DR = *pTxbuffer;
		pTxbuffer++;
		Len--;
	}

	//7. when Len becomes zero wait for TXE=1 and BTF=1 before generating the STOP condition
	//   Note: TXE=1 , BTF=1 , means that both SR and DR are empty and next transmission should begin
	//   when BTF=1 SCL will be stretched (pulled to LOW)

	while(! I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_TXE) );

	while(! I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_BTF) );


	//8. Generate STOP condition and master need not to wait for the completion of stop condition.
	//   Note: generating STOP, automatically clears the BTF
	if(Sr == I2C_DISABLE_SR )
		I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

}


/*********************************************************************
 * @fn      		  - I2C_ReceiveData
 *
 * @brief             - This function is used to receive data into a buffer
 *
 * @param[in]         - pI2CHandle : pointer to I2C handle structure
 * @param[in]         - pRxBuffer  : pointer to receive buffer
 * @param[in]         - Len        : length of data to be received
 * @param[in]         - SlaveAddr  : 7 bit address of slave device
 * @param[in]         - Sr         : repeated START control ( generate STOP / keep bus busy for a RESTART)
 *
 * @return            - none
 *
 * @Note              - This is blocking call. It internally uses SB, ADDR, RXNE
 *                      flags for receiving data.
 *                      before reading each frame from Data register.
 *                      Also handles data frame size while reception.
 *
 * @Abstraction       - Provides a single API to receive data buffer over SPI.
 *                      Application doesn't need to handle status flags or data register for receiving data.
 *
 *
 */
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle,uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr,uint8_t Sr)
{

	//1. Generate the START condition
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	//2. confirm that start generation is completed by checking the SB flag in the SR1
	//   Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	while( !  I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_SB)   );

	//3. Send the address of the slave with r/nw bit set to R(1) (total 8 bits )
	I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx,SlaveAddr);

	//4. wait until address phase is completed by checking the ADDR flag in teh SR1
	while( !  I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_ADDR)   );


	//procedure to read only 1 byte from slave
	if(Len == 1)
	{
		//Disable Acking
		I2C_ManageAcking(pI2CHandle->pI2Cx,I2C_ACK_DISABLE);


		//clear the ADDR flag
		I2C_ClearADDRFlag(pI2CHandle);

		//wait until  RXNE becomes 1
		while(! I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_RXNE) );

		//generate STOP condition
		if(Sr == I2C_DISABLE_SR )
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

		//read data in to buffer
		*pRxBuffer = pI2CHandle->pI2Cx->DR;

	}


    //procedure to read data from slave when Len > 1
	if(Len > 1)
	{
		//clear the ADDR flag: ACK set because RxLen > 1
		I2C_ClearADDRFlag(pI2CHandle);

		// Loop behavior (simplified multi-byte receive):
		//
		// - Receive initial bytes while ACK = 1
		// - When 2 bytes remain (i == 2):
		//     * Disable ACK to prepare NACK for final byte
		//     * Generate STOP if required
		// - Read data from DR into buffer
		//
		// Note: RXNE is used here for simplicity, but BTF-based handling (with clock streching)
		// is the recommended approach in the reference manual for timing safety.


		//read the data until Len becomes zero
		for ( uint32_t i = Len ; i > 0 ; i--)
		{
			//wait until RXNE becomes 1
			while(! I2C_GetFlagStatus(pI2CHandle->pI2Cx,I2C_FLAG_RXNE) );

			if(i == 2) //if last 2 bytes are remaining
			{
				//Disable Acking
				I2C_ManageAcking(pI2CHandle->pI2Cx,I2C_ACK_DISABLE);

				//generate STOP condition
				if(Sr == I2C_DISABLE_SR )
					I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

			}

			//read the data from data register in to buffer
			*pRxBuffer = pI2CHandle->pI2Cx->DR;

			//increment the buffer address
			pRxBuffer++;

		}

	}

	//re-enable ACKing
	if(pI2CHandle->I2C_Config.I2C_AckControl == I2C_ACK_ENABLE)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx,I2C_ACK_ENABLE);
	}

}

/*********************************************************************
 * @fn      		  - I2C_ManageAcking
 *
 * @brief             - This function is used to set or reset ACK
 *
 * @param[in]         - pI2Cx  : pointer to I2C register definition structure
 * @param[in]         - EnorDi : Enable or Disable
 *
 * @return            - none
 *
 * @Note              - Configures the ACK bit inside CR1 register for managing ACK
 *
 * @Abstraction       - Provides a single API to manage ACK without direct register access.
 *
 *
 */

void I2C_ManageAcking(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if(EnorDi == I2C_ACK_ENABLE)
	{
		//enable the ack
		pI2Cx->CR1 |= ( 1 << I2C_CR1_ACK);
	}else
	{
		//disable the ack
		pI2Cx->CR1 &= ~( 1 << I2C_CR1_ACK);
	}
}

/*********************************************************************
 * @fn      		  - I2C_IRQInterruptConfig
 *
 * @brief             - This function is used to enable or disable an interrupt request
 *
 * @param[in]         - IRQNumber: IRQ number
 * @param[in]         - EnorDi   : Enable or Disable.
 *
 * @return            - none
 *
 * @Note              -This function uses the ISER registers for enabling
 *                      and ICER registers for disabling a particular IRQ
 *
 * @Abstraction       - Provides a driver API for the application to
 *                      control an IRQ without accessing registers.
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{

	// Interrupt Set Enable Resgister (ISER) is used for enabling an IRQ
	// - each registers handle 32 IRQs
	// - ISER[0] = IRQ 0-IRQ 31
	// - ISER[1] = IRQ 32-IRQ 63
	// - ISER[2] = IRQ 64-IRQ 95
	// - each bit corresponds to one interrupt line

	if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			//program ISER0 register
			*NVIC_ISER0 |= ( 1 << IRQNumber );

		}else if(IRQNumber > 31 && IRQNumber < 64 ) //32 to 63
		{
			//program ISER1 register
			*NVIC_ISER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96 )
		{
			//program ISER2 register //64 to 95
			*NVIC_ISER2 |= ( 1 << (IRQNumber % 64) );
		}
	}else
	{
		// Interrupt Clear Enable Resgister (ICER) is used for disabling an IRQ
	    // - each registers handle 32 IRQs
		// - ICER[0] = IRQ 0-IRQ 31
		// - ICER[1] = IRQ 32-IRQ 63
		// - ICER[2] = IRQ 64-IRQ 95
	    // - each bit corresponds to one interrupt line

		if(IRQNumber <= 31)
		{
			//program ICER0 register
			*NVIC_ICER0 |= ( 1 << IRQNumber );
		}else if(IRQNumber > 31 && IRQNumber < 64 )
		{
			//program ICER1 register
			*NVIC_ICER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 6 && IRQNumber < 96 )
		{
			//program ICER2 register
			*NVIC_ICER2 |= ( 1 << (IRQNumber % 64) );
		}
	}

}


/*********************************************************************
 * @fn      		  - I2C_IRQPriorityConfig
 *
 * @brief             - This function configures the priority for an I2C IRQ
 *
 * @param[in]         - IRQNumber  : IRQ number
 * @param[in]         - IRQPriority: IRQ priority value
 *
 * @return            - none
 *
 * @Note              - STM32F4 implements only the upper priority bits of each
 *                      8-bit NVIC priority field. The number of implemented bits
 *                      is defined by NO_PR_BITS_IMPLEMENTED.
 *
 * @Abstraction       - This API is used by the application to set the priority of a specific IRQ.
 *                      Internally modifies the corresponding bit fields in the IPRx register,
 *                      hiding direct register access from the user.
 *
 */
void I2C_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority)
{
	// - Each IPR register handles 4 IRQ lines
	// - An 8 bit priority feild is allocated for each IRQ
	// - STM32F4 uses upper 4 bits of the field to set the priority
	// Examples:
	// - IRQ0  - IPR[0][7:0]
	// - IRQ1  - IPR[0][15:8]
	// - IRQ2  - IPR[0][23:16]
	// - IRQ3  - IPR[0][24:31]
	// - IRQ4  - IPR[1][7:0]

	//1. first lets find out the ipr register
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section  = IRQNumber %4 ;

	uint8_t shift_amount = ( 8 * iprx_section) + ( 8 - NO_PR_BITS_IMPLEMENTED) ;

	*(  NVIC_PR_BASE_ADDR + iprx ) |=  ( IRQPriority << shift_amount );

}

/*********************************************************************
 * @fn      		  - I2C_MasterSendDataIT
 *
 * @brief             - This function starts an interrupt-driven I2C data transmission.
 *
 * @param[in]         - pI2CHandle  : pointer to I2C Handle structure
 * @param[in]         - pTxBuffer   : pointer to transmit buffer
 * @param[in]         - Len         : length of data to be transmitted
 * @param[in]         - SlaveAddr   : 7 bit address of slave device
 * @param[in]         - Sr          : repeated START control ( generate STOP / keep bus busy for a RESTART)
 *
 * @return            - state       : data transmission state before the function call.
 *
 * @Note              - This function only initiates a data transmission,
 *                      generates a START condition,  marks the state as busy in transmission,
 *                      stores the transmit buffer information and slave address and enables the interrupts.
 *                      Actual data transfer is handled by I2C_MasterHandleTXEInterrupt in the IRQ Handler.
 *
 * @Abstraction       - The function hides the register-level interrupt configurations
 *                      and internal transmission state management from the application.
 *
 */
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle,uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr,uint8_t Sr)
{
	uint8_t busystate = pI2CHandle->TxRxState;

	if( (busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
	{
		pI2CHandle->pTxBuffer = pTxBuffer;
		pI2CHandle->TxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;

		//Implement code to Generate START Condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		//Implement the code to enable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= ( 1 << I2C_CR2_ITBUFEN);

		//Implement the code to enable ITEVFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= ( 1 << I2C_CR2_ITEVTEN);

		//Implement the code to enable ITERREN Control Bit
		pI2CHandle->pI2Cx->CR2 |= ( 1 << I2C_CR2_ITERREN);

	}

	return busystate;
}


/*********************************************************************
 * @fn      		  - I2C_MasterReceiveDataIT
 *
 * @brief             - This function initiates an interrupt-driven I2C data reception.
 *
 * @param[in]         - pI2CHandle  : pointer to I2C Handle structure
 * @param[in]         - pRxBuffer   : pointer to receive buffer
 * @param[in]         - Len         : length of data to be transmitted
 * @param[in]         - SlaveAddr   : 7 bit address of slave device
 * @param[in]         - Sr          : repeated START control ( generate STOP / keep bus busy for a RESTART)
 *
 * @return            - state       : data transmission state before the function call.
 *
 * @Note              - This function only initiates a data reception,
 *                      generates a START condition,  marks the state as busy in transmission,
 *                      stores the transmit buffer information and slave address and enables the interrupts.
 *                      Actual data reception is handled by I2C_MasterHandleRXNEInterrupt in the IRQ Handler.
 *
 * @Abstraction       - The function hides the register-level interrupt configurations
 *                      and internal transmission state management from the application.
 *
 */
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle,uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr,uint8_t Sr)
{
	uint8_t busystate = pI2CHandle->TxRxState;

	if( (busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
	{
		pI2CHandle->pRxBuffer = pRxBuffer;
		pI2CHandle->RxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
		pI2CHandle->RxSize = Len;
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;

		//Implement code to Generate START Condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		//Implement the code to enable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= ( 1 << I2C_CR2_ITBUFEN);

		//Implement the code to enable ITEVFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= ( 1 << I2C_CR2_ITEVTEN);

		//Implement the code to enable ITERREN Control Bit
		pI2CHandle->pI2Cx->CR2 |= ( 1 << I2C_CR2_ITERREN);
	}

	return busystate;
}
/*
 * Note : * This function is called by the I2C_EV_IRQHandling function when a TXE event occurs.
 *        * It writes data into Data Register from the TxBuffer untill data length becomes zero
 */
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle )
{

	if(pI2CHandle->TxLen > 0)
	{
		//1. load the data in to DR
		pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);

		//2. decrement the TxLen
		pI2CHandle->TxLen--;

		//3. Increment the buffer address
		pI2CHandle->pTxBuffer++;

	}

}
/*
 * Note : * This function is called by the I2C_EV_IRQHandling function when an RXNE event occurs.
 *        * It stores received data into the buffer and sets NACK for last byte.
 *        * Calls I2C_CloseReceiveData, generates STOP condition and notifies
 *          the application after rceiving last byte.
 *
 */
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle )
{
	//We have to do the data reception
	if(pI2CHandle->RxSize == 1)
	{
		*pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
		pI2CHandle->RxLen--;

	}


	if(pI2CHandle->RxSize > 1)
	{
		if(pI2CHandle->RxLen == 2)
		{
			//clear the ack bit: Set NACK for last byte.
			I2C_ManageAcking(pI2CHandle->pI2Cx,DISABLE);
		}

			//read DR
			*pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
			pI2CHandle->pRxBuffer++;
			pI2CHandle->RxLen--;
	}

	if(pI2CHandle->RxLen == 0 )
	{
		//close the I2C data reception and notify the application

		//1. generate the stop condition
		if(pI2CHandle->Sr == I2C_DISABLE_SR)
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

		//2 . Close the I2C rx
		I2C_CloseReceiveData(pI2CHandle);

		//3. Notify the application
		I2C_ApplicationEventCallback(pI2CHandle,I2C_EV_RX_CMPLT);
	}
}

/*
 * Note : * This function is called by the MasterHandleRXNEInterrupt function when last byte is received
 *        * For closing I2C reception, it performs:
 *
 *                                    - Resetting buffer information (pointer, length, size)
 *                                    - Disables interrupts
 *                                    - Re Enables ACK
 *                                    - Sets the state as READY
 */

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
	//Implement the code to disable ITBUFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);

	//Implement the code to disable ITEVFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVTEN);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pRxBuffer = NULL;
	pI2CHandle->RxLen = 0;
	pI2CHandle->RxSize = 0;

	if(pI2CHandle->I2C_Config.I2C_AckControl == I2C_ACK_ENABLE)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx,ENABLE);
	}

}

/*
 * Note : * This function is called by the I2C_EV_IRQHandling function at end of a transmission (BTF = 1)
 *        * For closing I2C transmission, it performs:
 *
 *                                    - Resetting buffer information (pointer, length)
 *                                    - Disables interrupts
 *                                    - Sets the state as READY
 */


void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
	//Implement the code to disable ITBUFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);

	//Implement the code to disable ITEVFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVTEN);


	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pTxBuffer = NULL;
	pI2CHandle->TxLen = 0;
}


void I2C_SlaveSendData(I2C_RegDef_t *pI2C,uint8_t data)
{
	pI2C->DR = data;
}

uint8_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2C)
{
    return (uint8_t) pI2C->DR;
}

/*********************************************************************
 * @fn      		  - I2C_EV_IRQHandling
 *
 * @brief             - This function handles I2C interrupt events for data
 *                      transmission and reception
 *
 * @param[in]         - pI2CHandle: pointer to I2C Handle structure
 *
 * @return            - none
 *
 * @Note              - This function manages interrupt sources such as SB,ADDR,BTF,STOPF,TXE and RXNE.
 *
 *
 * @Abstraction       - Acts as core of interrupt-driven I@c driver
 *                      it hides internal state management, ISR handling
 *                      and notifies only high level events to the application.
 *
 */

void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	//Interrupt handling for both master and slave mode of a device

	uint32_t temp1, temp2, temp3;

	temp1   = pI2CHandle->pI2Cx->CR2 & ( 1 << I2C_CR2_ITEVTEN) ;
	temp2   = pI2CHandle->pI2Cx->CR2 & ( 1 << I2C_CR2_ITBUFEN) ;

	temp3  = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_SB);


	//1. Handle For interrupt generated by SB event
	/*****************************************************************************************************/

	//	Note : SB flag is only applicable in Master mode
	if(temp1 && temp3)
	{
		//The interrupt is generated because of SB event
		//This block will not be executed in slave mode because for slave SB is always zero
		//In this block lets executed the address phase
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx,pI2CHandle->DevAddr);
		}else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX )
		{
			I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx,pI2CHandle->DevAddr);
		}
	}

	temp3  = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_ADDR);


	//2. Handle For interrupt generated by ADDR event
	/*****************************************************************************************************/

	//Note : When master mode : Address is sent
	//		 When Slave mode   : Address matched with own address
	if(temp1 && temp3)
	{
		// interrupt is generated because of ADDR event
		I2C_ClearADDRFlag(pI2CHandle);
	}

	temp3  = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_BTF);


	//3. Handle For interrupt generated by BTF(Byte Transfer Finished) event
	/*****************************************************************************************************/

	if(temp1 && temp3)
	{
		//BTF flag is set
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			//make sure that TXE is also set .
			if(pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_TXE) )
			{
				//BTF, TXE = 1
				if(pI2CHandle->TxLen == 0 )
				{
					//1. generate the STOP condition
					if(pI2CHandle->Sr == I2C_DISABLE_SR)
						I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

					//2. reset all the member elements of the handle structure.
					I2C_CloseSendData(pI2CHandle);

					//3. notify the application about transmission complete
					I2C_ApplicationEventCallback(pI2CHandle,I2C_EV_TX_CMPLT);

				}
			}

		}else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX )
		{
			;
		}
	}

	temp3  = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_STOPF);


	//4. Handle For interrupt generated by STOPF event
	/*****************************************************************************************************/

	// Note : Stop detection flag is applicable only slave mode . For master this flag will never be set
	//The below code block will not be executed by the master since STOPF will not set in master mode
	if(temp1 && temp3)
	{
		//STOF flag is set
		//Clear the STOPF ( i.e 1) read SR1 2) Write to CR1 )

		(void)pI2CHandle->pI2Cx->SR1;
		pI2CHandle->pI2Cx->CR1 |= 0x0000;

		//Notify the application that STOP is detected
		I2C_ApplicationEventCallback(pI2CHandle,I2C_EV_STOP);
	}


	temp3  = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_TXE);


	//5. Handle For interrupt generated by TXE event
	/*****************************************************************************************************/

	if(temp1 && temp2 && temp3)
	{
		//Check for device mode
		if(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_MSL))
		{
			//TXE flag is set
			//We have to do the data transmission
			if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
			{
				I2C_MasterHandleTXEInterrupt(pI2CHandle);
			}
		}else
		{
			//slave
			//make sure that the slave is really in transmitter mode
		    if(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_TRA))
		    {
		    	I2C_ApplicationEventCallback(pI2CHandle,I2C_EV_DATA_REQ);
		    }
		}
	}

	temp3  = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_RXNE);


	//6. Handle For interrupt generated by RXNE event
	/*****************************************************************************************************/

	if(temp1 && temp2 && temp3)
	{
		//check device mode .
		if(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_MSL))
		{
			//The device is master

			//RXNE flag is set
			if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
			{
				I2C_MasterHandleRXNEInterrupt(pI2CHandle);

			}

		}else
		{
			//slave
			//make sure that the slave is really in receiver mode
			if(!(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_TRA)))
			{
				I2C_ApplicationEventCallback(pI2CHandle,I2C_EV_DATA_RCV);
			}
		}
	}
}



/*********************************************************************
 * @fn      		  - I2C_ER_IRQHandling
 *
 * @brief             - This function handles interrupts generated by errors.( ARLO, AF, OVR, BERR)
 *
 * @param[in]         - pI2CHandle - pointer to I2C Handle structure.
 *
 * @return            - none
 *
 * @Note              - Complete the code also define these macros in the driver
						header file
						#define I2C_ERROR_BERR  3
						#define I2C_ERROR_ARLO  4
						#define I2C_ERROR_AF    5
						#define I2C_ERROR_OVR   6
						#define I2C_ERROR_TIMEOUT 7

 */

void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{

	uint32_t temp1,temp2;

    //Know the status of  ITERREN control bit in the CR2
	temp2 = (pI2CHandle->pI2Cx->CR2) & ( 1 << I2C_CR2_ITERREN);


/***********************Check for Bus error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1<< I2C_SR1_BERR);
	if(temp1  && temp2 )
	{
		//This is Bus error: Occurs due to misplaced START/STOP conditions

		//1) Implement the code to clear the buss error flag
		pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_BERR);

		//2) Generate STOP condition if I2C is busy in transaction in master mode
		if((pI2CHandle->pI2Cx->SR2 & I2C_SR2_MSL) && pI2CHandle->TxRxState != I2C_READY)
		{
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}

		//3) Close acive transactions
		if( pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
		}
		else if( pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
		}

		//4) Implement the code to notify the application about the error
	   I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_BERR);
	}

/***********************Check for arbitration lost error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_ARLO );
	if(temp1  && temp2)
	{
		//This is arbitration lost error: Occurs due to bus contention
		// - No stop generation needed because after ARLO,
		//   hardware automatically releases the bus, master switches to slave mode.

		//1)Implement the code to clear the arbitration lost error flag
		pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_ARLO);

		//2) Close acive transactions
		if( pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
		}
		else if( pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
		}

		//3) Implement the code to notify the application about the error
		I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_ARLO);

	}

/***********************Check for ACK failure  error************************************/

	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_AF);
	if(temp1  && temp2)
	{
		//This is ACK failure error: Occurs during transmission when receiver fails to give ACK

	    //1)Implement the code to clear the ACK failure error flag
		pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_AF);

		//2) Generate STOP condition if I2C is busy in transaction in master mode
		if((pI2CHandle->pI2Cx->SR2 & I2C_SR2_MSL) && pI2CHandle->TxRxState != I2C_READY)
		{
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}

		//3) Close transactions
		I2C_CloseSendData(pI2CHandle);

		//4) Implement the code to notify the application about the error
		I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_AF);
	}

/***********************Check for Overrun/underrun error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_OVR);
	if(temp1  && temp2)
	{
		//This is Overrun/underrun: Occurs when data not read/written in time in No clock strech mode

	    //1) Implement the code to clear the Overrun/underrun error flag
		uint32_t temp;
		temp = pI2CHandle->pI2Cx->SR1;
	    temp = pI2CHandle->pI2Cx->SR2;
		(void)temp;

		//2) Generate STOP condition if I2C is busy in transaction in master mode
		if((pI2CHandle->pI2Cx->SR2 & I2C_SR2_MSL) && pI2CHandle->TxRxState != I2C_READY)
		{
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}

		//3) Close acive transactions
		if( pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
		}
		else if( pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
		}

		//4) Implement the code to notify the application about the error
		I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_OVR);
	}

/***********************Check for Time out error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_TIMEOUT);
	if(temp1  && temp2)
	{
		//This is Time out error: Occurs in smbus mode

	    //1)Implement the code to clear the Time out error flag
		uint32_t temp;
		temp = pI2CHandle->pI2Cx->SR1;
		temp = pI2CHandle->pI2Cx->SR2;
		(void)temp;

		//2) Generate STOP condition if I2C is busy in transaction in master mode
		if((pI2CHandle->pI2Cx->SR2 & I2C_SR2_MSL) && pI2CHandle->TxRxState != I2C_READY)
		{
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}

		//3) Close acive transactions
		if( pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
		}
		else if( pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
		}

		//4) Implement the code to notify the application about the error
		I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_TIMEOUT);
	}

}



