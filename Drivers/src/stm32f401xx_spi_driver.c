/*
 * stm32f446xx_spi_driver.c
 *neeraj
 */

#include <stm32f401xx_spi_driver.h>

static void  spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void  spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void  spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle);

/*********************************************************************
 * @fn      		  - SPI_PeriClockControl
 *
 * @brief             - This function enables or disables peripheral clock for the given SPI peripheral
 *
 * @param[in]         - pSPIx: pointer to SPI register definition structure
 * @param[in]         - EnorDi: Enable or Disable
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API abstracts the clock enable registers in RCC peripheral from the application layer.
 *                      Internally uses APB1ENR and APB2ENR rgisters and configures corresponding bits.
 *
 */
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pSPIx == SPI1)
		{
			// Enables SPI1 clock by setting bit 12 in APB2ENR
			SPI1_PCLK_EN();
		}else if (pSPIx == SPI2)
		{
			// Enables SPI2 clock by setting bit 14 in APB1ENR
			SPI2_PCLK_EN();
		}else if (pSPIx == SPI3)
		{
			// Enables SPI3 clock by setting bit 15 in APB1ENR
			SPI3_PCLK_EN();
		}else if (pSPIx == SPI4)
		{
			// Enables SPI4 clock by setting bit 13 in APB2ENR
			SPI4_PCLK_EN();
		}
	}
	else
	{
		if(pSPIx == SPI1)
		{
			// Disables SPI1 clock by resetting bit 12 in APB2ENR
			SPI1_PCLK_DI();
		}else if (pSPIx == SPI2)
		{
			// Disables SPI2 clock by resetting bit 14 in APB1ENR
			SPI2_PCLK_DI();
		}else if (pSPIx == SPI3)
		{
			// Disables SPI3 clock by resetting bit 15 in APB1ENR
			SPI3_PCLK_DI();
		}else if (pSPIx == SPI4)
		{
			// Disables SPI4 clock by resetting bit 13 in APB2ENR
			SPI4_PCLK_DI();
		}
	}
}


/*********************************************************************
 * @fn      		  - SPI_Init
 *
 * @brief             - This function initializes the given SPI peripheral.
 *
 * @param[in]         - pSPIHandle: pointer to SPI Handle structure.
 *
 * @return            - none
 *
 * @Note              - This function configures the SPI device mode(Master/Slave), communication parameters such as
 *                      SCK clock speed, data frame format(16 bits / 8 bits), clock polarity, clock phase,
 *                      slave management(software /hardware) and bus configuration.
 *
 * @Abstraction       - This API is used by the application to initialize
 *                      and configure the SPI peripheral. Internally it
 *                      programs the required SPI registers based on the
 *                      user configurations present in the handle structure.
 *
 */


void SPI_Init(SPI_Handle_t *pSPIHandle)
{

	//peripheral clock enable

	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

	//first lets configure the SPI_CR1 register

	uint32_t tempreg = 0;

	//1. configure the device mode
	tempreg |= pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR;

	//2. Configure the bus config
	if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
	{
		// In Full duplex configuration,
		// - MOSI - For transmitting data
		// - MISO - For receiving data
		// Enables simultaneous data transmission and reception.

		// bidi mode should be cleared
		tempreg &= ~( 1 << SPI_CR1_BIDIMODE);

	}else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
	{
		// In bidirectional Mode (Half-Duplex),
		// - MOSI - Used as a single data line in Master mode, MISO - ignored
		// - MISO - Used as a single data line in Slave mode,  MOSI - ignored
		// Data transmitted in one direction at a time.
		// Data direction depends on BIDIOE bit

		//bidi mode should be set
		tempreg |= ( 1 << SPI_CR1_BIDIMODE);
	}else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
	{
		// In Simplex receive-only Mode,
		//  - Output pin will be disabled (MOSI in master mode & MISO in slave mode)
		// Communication only in one direction

		//BIDI mode should be cleared
		tempreg &= ~( 1 << SPI_CR1_BIDIMODE);
		//RXONLY bit must be set
		tempreg |= ( 1 << SPI_CR1_RXONLY);
	}

	// 3. Configure the spi serial clock speed (baud rate)

	// SCK clock = Peripheral clock / prescaler
	// SCK prescaler values = {2, 4, 8, 16, 32, 64, 128, 256}
	// BR[2:0] bits in CR1 selects prescaler.
	tempreg |= pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR;

	//4.  Configure the DFF

	// DFF bit in CR1 register is used to set data frame size
	//  0 - 8  bits
	//  1 - 16 bits
	tempreg |= pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_DFF;

	//5. configure the CPOL

	// CPOL bit selects clock polarity
	//  0 - clock is LOW when idle
	//  1 - clock is HIGH when idle
	tempreg |= pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL;

	//6 . configure the CPHA

	// CPHA bit selects clock phase
	//  0 - data sampled on first clock edge
    //  1 - data sampled on second clock edge
	tempreg |= pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA;

	// Select slave management

	// SSM bit is used to enable/disable software slave management
	//  0 - NSS managed by hardware
	//  1 - NSS managed by software
	tempreg |= pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM;

	pSPIHandle->pSPIx->CR1 = tempreg;

}

/*********************************************************************
 * @fn      		  - SPI_DeInit
 *
 * @brief             - This function resets the given SPI peripheral to its default state
 *
 * @param[in]         - pSPIx: pointer to SPI register definition structure
 *
 * @return            - none
 *
 * @Note              - This function uses the RCC reset registers to reset
 *                      the SPI peripheral. After reset, all SPI registers
 *                      return to their default values.
 *
 * @Abstraction       - This API resets SPI peripheral using appropriate reset register and bits in the RCC peripheral
 *                      it hides the SPI reset mechanism from application layer
 *
 *
 */
void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
	// Reset mechansim:
	//
	// SPI peripheral is reset using:
	//   - APB1RSTR for SPI2 and SPI3
	//   - APB2RSTR for SPI1 and SPI4
	//
	// Reset sequence:
	// 1. Set the corresponding bit in register.
	//    This forces the peripheral into its reset state.
	// 2. Clear the bit.
	//    This releases the peripheral from reset.
	//
	// After reset, all SPI registers return to their default values.

	if(pSPIx == SPI1)
	{
		SPI1_REG_RESET();
	}
	else if(pSPIx == SPI2)
	{
		SPI2_REG_RESET();
	}
	else if(pSPIx == SPI3)
	{
		SPI3_REG_RESET();
	}
	else if(pSPIx == SPI4)
	{
		SPI4_REG_RESET();
	}

}
/*********************************************************************
 * @fn      		  - SPI_GetFlagStatus
 *
 * @brief             - This function is used to get the status of flags
 *
 * @param[in]         - pSPIx   : pointer to SPI register definition structure
 * @param[in]         - FlagName: name of status flag
 *
 * @return            - status of flag (1 or 0)
 *
 * @Note              - none
 *
 * @Abstraction       - This API internally reads the
 *                      corresponding bit in the SPI status register (SR),
 *                      hiding direct register access from the user.
 *
 */
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx , uint32_t FlagName)
{
	if(pSPIx->SR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*********************************************************************
 * @fn      		  - SPI_SendData
 *
 * @brief             - This function is used to transmit a complete data buffer
 *
 * @param[in]         - pSPIx    : pointer to SPI register definition structure.
 * @param[in]         - pTxBuffer: pointer to transmit buffer
 * @param[in]         - Len      : length of data to be transmitted
 *
 * @return            - none
 *
 * @Note              - This is blocking call. It internally uses TXE, BSY status flags
 *                      and Data register for transmitting each data frame.
 *                      Also handles data frame size while transmission.
 *
 * @Abstraction       - Provides a single API to transmit data buffer over SPI.
 *                      Application doesn't need to handle status flags or data register for transmitting data.
 *
 */
void SPI_SendData(SPI_RegDef_t *pSPIx,uint8_t *pTxBuffer, uint32_t Len)
{
	while(Len > 0)
	{
		//1. wait until TXE is set ( wait till transmit buffer is empty)
		while(SPI_GetFlagStatus(pSPIx,SPI_TXE_FLAG)  == FLAG_RESET );

		//2. check the DFF bit in CR1 ( transmit data based of frame size )
		if( (pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
		{
			//16 bit DFF
			//1. load the data in to the DR
			pSPIx->DR =   *((uint16_t*)pTxBuffer);
			Len--;
			Len--;
			(uint16_t*)pTxBuffer++;
		}else
		{
			//8 bit DFF
			pSPIx->DR =   *pTxBuffer;
			Len--;
			pTxBuffer++;
		}
	}
	// wait until BSY is reset ( wait till transmission is complete)
	while(SPI_GetFlagStatus(pSPIx,SPI_BUSY_FLAG));

}

/*********************************************************************
 * @fn      		  - SPI_ReceiveData
 *
 * @brief             - This function is used to receive data into a buffer
 *
 * @param[in]         - pSPIx    : pointer to SPI register definition structure.
 * @param[in]         - pRxBuffer: pointer to receive buffer
 * @param[in]         - Len      : length of data to be received
 *
 * @return            - none
 *
 * @Note              - This is blocking call. It internally waits for RXNE flag
 *                      before reading each frame from Data register.
 *                      Also handles data frame size while reception.
 *
 * @Abstraction       - Provides a single API to receive data buffer over SPI.
 *                      Application doesn't need to handle status flags or data register for receiving data.
 *
 *
 */
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	while(Len > 0)
		{
			//1. wait until RXNE is set
			while(SPI_GetFlagStatus(pSPIx,SPI_RXNE_FLAG)  == (uint8_t)FLAG_RESET );

			//2. check the DFF bit in CR1
			if( (pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
			{
				//16 bit DFF
				//1. load the data from DR to Rxbuffer address
				 *((uint16_t*)pRxBuffer) = pSPIx->DR ;
				Len--;
				Len--;
				(uint16_t*)pRxBuffer++;
			}else
			{
				//8 bit DFF
				*(pRxBuffer) = pSPIx->DR ;
				Len--;
				pRxBuffer++;
			}
		}

}


/*********************************************************************
 * @fn      		  - SPI_PeripheralControl
 *
 * @brief             - This function enables or disables the given SPI peripheral.
 *
 * @param[in]         - pSPIx : pointer to SPI register definition structure
 * @param[in]         - EnorDi: Enable or Disable
 *
 * @return            - none
 *
 * @Note              - This function internally uses SPE bit in CR1 register to enable or disable SPI.
 *
 * @Abstraction       - Provides a clean API for application to control SPI peripheral
 *                      without direct register access.
 *
 */
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if(EnOrDi == ENABLE)
	{
		// CR1_SPE = 1 -> SPI Enable
		pSPIx->CR1 |=  (1 << SPI_CR1_SPE);
	}else
	{
		// CR1_SPE = 0 -> SPI Disable
		pSPIx->CR1 &=  ~(1 << SPI_CR1_SPE);
	}


}


/*********************************************************************
 * @fn      		  - SPI_SSIConfig
 *
 * @brief             - This function configures the SSI bit in CR1 register
 *
 * @param[in]         - pSPIx : pointer to SPI register definition structure
 * @param[in]         - EnorDi: Enable or Disable
 *
 * @return            - none
 *
 * @Note              - This bit is kept HIGH is master mode ( Avoids Modefault and keeps SPI enabled )
 *                      bit is kept LOW in slave mode        ( Slave considers itself selected )
 *
 * @Abstraction       - Provides a clean API for controlling the SSI bit without
 *                      direct register manipulation. This allows the application
 *                      to manage internal NSS logic when using software slave management
 */
void  SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if(EnOrDi == ENABLE)
	{
		pSPIx->CR1 |=  (1 << SPI_CR1_SSI);
	}else
	{
		pSPIx->CR1 &=  ~(1 << SPI_CR1_SSI);
	}


}


/*********************************************************************
 * @fn      		  - SPI_SSOEConfig
 *
 * @brief             - This function configures the SSOE bit in CR2 register
 *
 * @param[in]         - pSPIx : pointer to SPI register definition structure
 * @param[in]         - EnorDi: Enable or Disable
 *
 * @return            - none
 *
 * @Note              - This bit is set HIGH in master mode
 *                           - To automatically drive NSS pin for slave selection
 *                           - driven LOW when SPI enabled
 *                           . driven HIGH when SPI disabled
 *                      bit is ignored in slave mode        ( NSS pin acts as an input pin )
 *
 * @Abstraction       - Provides a clean API for controlling the SSOE bit without
 *                      direct register manipulation. This allows the SPI peripheral
 *                      to manage NSS pin when using hardware slave management.
 */
void  SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if(EnOrDi == ENABLE)
	{
		pSPIx->CR2 |=  (1 << SPI_CR2_SSOE);
	}else
	{
		pSPIx->CR2 &=  ~(1 << SPI_CR2_SSOE);
	}


}



/*********************************************************************
 * @fn      		  - SPI_IRQInterruptConfig
 *
 * @brief             - This function is used to enable or disable an interrupt request
 *
 * @param[in]         - IRQNumber: IRQ number
 * @param[in]         - EnorDi   : Enable or Disable.
 *
 * @return            - none
 *
 * @Note              - This function uses the ISER registers for enabling
 *                      and ICER registers for disabling a particular IRQ
 *
 * @Abstraction       - Provides a driver API for the application  to configure
 *                      the NVIC registers and control an IRQ without accessing registers.
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{

	if(EnorDi == ENABLE)
	{
		// Interrupt Set Enable Resgister (ISER) is used for enabling an IRQ
		// - each registers handle 32 IRQs
		// - ISER[0] = IRQ 0-IRQ 31
		// - ISER[1] = IRQ 32-IRQ 63
		// - ISER[2] = IRQ 64-IRQ 95
		// - each bit corresponds to one interrupt line

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
	}else// IRQ Disable
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
		else if(IRQNumber >= 64 && IRQNumber < 96 )
		{
			//program ICER2 register
			*NVIC_ICER2 |= ( 1 << (IRQNumber % 64) );
		}
	}

}


/*********************************************************************
 * @fn      		  - SPI_IRQPriorityConfig
 *
 * @brief             - This function configures the priority for an SPI IRQ
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
void SPI_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority)
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
 * @fn      		  - SPI_SendDataIT
 *
 * @brief             - This function starts an interrupt-driven SPI data transmission.
 *
 * @param[in]         - pSPIHandle  : pointer to USART Handle structure
 * @param[in]         - pTxBuffer   : pointer to transmit buffer
 * @param[in]         - Len         : length of data to be transmitted
 *
 * @return            - state       : data transmission state before the function call.
 *
 * @Note              - This function only initiates a data transmission,
 *                      it stores the transmit buffer information, marks the state as busy in transmission,
 *                      and enables TXE and overrun interrupts.
 *                      Actual data transfer is handled by spi_txe_interrupt_handle in the IRQ Handler.
 *
 * @Abstraction       - The function hides the register-level interrupt configurations
 *                      and internal transmission state management from the application.
 *
 */

uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle,uint8_t *pTxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->TxState;

	if(state != SPI_BUSY_IN_TX)
	{
		//1 . Save the Tx buffer address and Len information in the handle structure
		pSPIHandle->pTxBuffer = pTxBuffer;
		pSPIHandle->TxLen = Len;

		//2.  Mark the SPI state as busy in transmission so that
		//    no other code can take over same SPI peripheral until transmission is over
		pSPIHandle->TxState = SPI_BUSY_IN_TX;

		//3. Enable the TXEIE control bit to get interrupt whenever TXE flag is set in SR
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_TXEIE );

		//4. Enable ERRIE control bit to detect whenever an overrun error occur
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_ERRIE );

	}

	return state;
}
/*********************************************************************
 * @fn      		  - SPI_ReceiveDataIT
 *
 * @brief             - This function initiates an interrupt-driven SPI data reception.
 *
 * @param[in]         - pSPIHandle  : pointer to SPI Handle structure
 * @param[in]         - pRxBuffer   : pointer to receive buffer
 * @param[in]         - Len         : length of data to be received
 *
 * @return            - state       : data reception state before the function call
 *
 * @Note              - This function starts a data reception in an interrupt-driven manner,
 *                      it stores the buffer pointer and length in the handle structure,
 *                      sets the reception state as busy and enables the RXNE and overrun interrupt
 *                      Actual data reception is performed by spi_rxne_interrupt_handle in the IRQ handler.
 *
 * @Abstraction       - The function hides the register-level interrupt configurations
 *                      and internal reception state management from the application.
 */

uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->RxState;

	if(state != SPI_BUSY_IN_RX)
	{
		//1 . Save the Rx buffer address and Len information in some global variables
		pSPIHandle->pRxBuffer = pRxBuffer;
		pSPIHandle->RxLen = Len;

		//2.  Mark the SPI state as busy in reception so that
		//    no other code can take over same SPI peripheral until reception is over
		pSPIHandle->RxState = SPI_BUSY_IN_RX;

		//3. Enable the RXNEIE control bit to get interrupt whenever RXNEIE flag is set in SR
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_RXNEIE );

		//4. Enable ERRIE control bit to detect whenever an overrun error occur
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_ERRIE );

	}
	return state;
}

/*********************************************************************
 * @fn      		  - SPI_IRQHandler
 *
 * @brief             - This function handles SPI interrupt events for data
 *                      transmission, reception, and overrun error.
 *
 * @param[in]         - pSPIHandle: pointer to USART Handle structure
 *
 * @return            - none
 *
 * @Note              - This function manages interrupt sources such as TXE,RXNE,
 *                      and overrun flag.
 *                      Performs
 *                            - data transmission using TXE interrupt.
 *                            - reception using RXNE interrupt
 *                            - overrun error detection
 *                      It also notifies the application layer by calling
 *                      SPI_ApplicationEventCallback() for significant
 *                      events such as TX complete, RX complete, and errors.
 *
 *                      Note that both events and errors in SPI are managed by the same IRQ handler.
 *
 * @Abstraction       - Acts as core of interrupt-driven SPI driver
 *                      it hides internal RX/TX state management, ISR handling
 *                      and notifies only high level events to the application.
 *
 */
void SPI_IRQHandling(SPI_Handle_t *pHandle)
{

	uint8_t temp1 , temp2;
	//first lets check for TXE
	temp1 = pHandle->pSPIx->SR & ( 1 << SPI_SR_TXE);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << SPI_CR2_TXEIE);

	if( temp1 && temp2)
	{
		//handle TXE
		spi_txe_interrupt_handle(pHandle);
	}

	// check for RXNE
	temp1 = pHandle->pSPIx->SR & ( 1 << SPI_SR_RXNE);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << SPI_CR2_RXNEIE);

	if( temp1 && temp2)
	{
		//handle RXNE
		spi_rxne_interrupt_handle(pHandle);
	}

	// check for ovr flag
	temp1 = pHandle->pSPIx->SR & ( 1 << SPI_SR_OVR);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << SPI_CR2_ERRIE);

	if( temp1 && temp2)
	{
		//handle ovr error
		spi_ovr_err_interrupt_handle(pHandle);
	}


}


//some helper function implementations

static void  spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	// check the DFF bit in CR1
	if( (pSPIHandle->pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
	{
		//16 bit DFF
		//1. load the data in to the DR
		pSPIHandle->pSPIx->DR =   *((uint16_t*)pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen--;
		pSPIHandle->TxLen--;
		(uint16_t*)pSPIHandle->pTxBuffer++;
	}else
	{
		//8 bit DFF
		pSPIHandle->pSPIx->DR =   *pSPIHandle->pTxBuffer;
		pSPIHandle->TxLen--;
		pSPIHandle->pTxBuffer++;
	}

	if(! pSPIHandle->TxLen)
	{
		//TxLen is zero , so close the spi transmission and inform the application that
		//TX is over.

		//this prevents interrupts from setting up of TXE flag
		SPI_CloseTransmission(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_TX_CMPLT);
	}

}


static void  spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	//do rxing as per the dff
	if(pSPIHandle->pSPIx->CR1 & ( 1 << 11))
	{
		//16 bit
		*((uint16_t*)pSPIHandle->pRxBuffer) = (uint16_t) pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen -= 2;
		pSPIHandle->pRxBuffer++;
		pSPIHandle->pRxBuffer++;

	}else
	{
		//8 bit
		*(pSPIHandle->pRxBuffer) = (uint8_t) pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen--;
		pSPIHandle->pRxBuffer++;
	}

	if(! pSPIHandle->RxLen)
	{
		//reception is complete
		SPI_CloseReception(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_RX_CMPLT);
	}

}


static void  spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	uint8_t temp;
	//1. clear the ovr flag
	if(pSPIHandle->TxState != SPI_BUSY_IN_TX)
	{
		temp = pSPIHandle->pSPIx->DR;
		temp = pSPIHandle->pSPIx->SR;
	}
	(void)temp;
	//2. inform the application
	SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_OVR_ERR);

}
/*********************************************************************
 * @fn        -  SPI_CloseTransmission
 *
 * @brief     -  Closes an interrupt-driven SPI transmission.
 *
 * @param[in] -  pSPIHandle : Pointer to SPI handle structure
 *
 * @return    -  none
 *
 * @note      -  Disables the TXE and overrun interrupt, resets tx buffer pointer
 *               and buffer length, and marks the SPI peripheral as ready.
 *
 */
void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle)
{
	// Clear TXIEIE control bit to disable TXE interrupt
	pSPIHandle->pSPIx->CR2 &= ~( 1 << SPI_CR2_TXEIE);
	// Clear ERRIE control bit to disable OVR interrupt
	pSPIHandle->pSPIx->CR2 &=~ (1<< SPI_CR2_ERRIE);

	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0;

	pSPIHandle->TxState = SPI_READY;

}
/*********************************************************************
 * @fn        -  SPI_CloseReception
 *
 * @brief     -  Closes an interrupt-driven SPI reception.
 *
 * @param[in] -  pSPIHandle : Pointer to SPI handle structure
 *
 * @return    -  none
 *
 * @note      -  Disables the RXNE and overrun interrupt, resets rx buffer pointer
 *               and buffer length, and marks the SPI peripheral as ready.
 *
 */
void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
	// Clear RXNEIE control bit to disable RXNE interrupt
	pSPIHandle->pSPIx->CR2 &= ~( 1 << SPI_CR2_RXNEIE);
	// Clear ERRIE control bit to disable OVR interrupt
	pSPIHandle->pSPIx->CR2 &=~ (1<< SPI_CR2_ERRIE);

	pSPIHandle->pRxBuffer   = NULL;
	pSPIHandle->RxLen       = 0;

	pSPIHandle->RxState     = SPI_READY;
}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
	uint8_t temp;
	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
}


/*********************************************************************
 * @fn      		  - SPI_ApplicationEventCallback
 *
 * @brief             - This is a weak call back function called by the IRQ Handler to notify
 *                      the application layer about SPI events.
 *
 * @param[in]         - pSPIHandle  : pointer to SPI Handle structure
 * @param[in]         - AppEv       : Event generated
 *
 * @return            - none
 *
 * @Note              - This is a weak function, the user
 *                      can redefine this function in the application layer to
 *                      implement custom logic for SPI events.
 *                      This function is called from an ISR, so it must be short and non-blocking
 *
 * @Abstraction       - Provides an interface between SPI driver and application,
 *                      allowing the application to handle  high level events and
 *                      implements custom logic withput modifying the driver.
 */
__weak void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle,uint8_t AppEv)
{

	//This is a weak implementation . the user application may override this function.
}






