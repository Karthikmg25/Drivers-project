/*
 * stm32f446xx_usart_driver.c
 * USART/UART Driver for STM32F446xx MCU
 * Author: Neeraj
 */

#include <stm32f401xx_usart_driver.h>


/*********************************************************************
 * @fn      		  - USART_SetBaudRate
 *
 * @brief             - This function sets the required baud rate for the given USART peripheral
 *
 * @param[in]         - pUSARTx:  pointer to USART register definition structure
 * @param[in]         - BaudRate: communication baud rate
 *
 * @return            - none
 *
 * @Note              - This function calculates and configures the usart Baud Rate Register (BRR).
 *                      BRR stores the integer and fraction part of USART div value,
 *                            USART div = Peripheral clock frequency / (Oversampling rate * BaudRate)
 *
 *                      Upper 12 bits(BRR[11:0]) store the mantissa
 *                      Lower 4 bits (BRR[3:0])  store fraction part when oversampling is 16
 *                      Lower 3 bits (BRR[2:0])  store fraction part when oversampling is 8
 *
 * @Abstraction       - This function abstracts the baud rate calculation and BRR register configuration,
 *                      allowing the user to set the baud rate without direct register accessing.
 */

void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate)
{

	//Variable to hold the APB clock
	uint32_t PCLKx;

	uint32_t usartdiv;

	//variables to hold Mantissa and Fraction values
	uint32_t M_part,F_part;

  uint32_t tempreg=0;

  //Get the value of APB bus clock in to the variable PCLKx
  if(pUSARTx == USART1 || pUSARTx == USART6)
  {
	   //USART1 and USART6 are hanging on APB2 bus
	   PCLKx = RCC_GetPCLK2Value();
  }else
  {
	  //USART2 is hanging on APB1 bus
	   PCLKx = RCC_GetPCLK1Value();
  }
  // Use Fixed point arithmetic to avoid floating point operations

  // usartdiv is scaled by 100 to preserve 2 digits of precision,
  // Mantissa part = usartdiv/100
  // Fraction part= (usartdiv - (mantissa * 100)) -> This value represents the actual fraction part of usartdiv scaled by 100.
  // for over sampling by 16,
  //                  BRR_fraction = ((fraction * 16) + 50) / 100 -> maps the fraction part into the 4 bit field
  //                                                              -> add 50 for rounding to the nearest integer
  // for over sampling by 8,
  //                  BRR_fraction = ((fraction * 8) + 50) / 100 -> maps the fraction part into the 3 bit field
  //                                                              -> add 50 for rounding to the nearest integer

  //Check for OVER8 configuration bit
  if(pUSARTx->CR1 & (1 << USART_CR1_OVER8))
  {
	   //OVER8 = 1 , over sampling by 8
	   usartdiv = ((25 * PCLKx) / (2 *BaudRate));
  }else
  {
	   //over sampling by 16
	   usartdiv = ((25 * PCLKx) / (4 *BaudRate));
  }

  //Calculate the Mantissa part
  M_part = usartdiv/100;

  //Place the Mantissa part in appropriate bit position . refer USART_BRR
  tempreg |= M_part << 4;

  //Extract the fraction part
  F_part = (usartdiv - (M_part * 100));

  //Calculate the final fractional
  if(pUSARTx->CR1 & ( 1 << USART_CR1_OVER8))
   {
	  //OVER8 = 1 , over sampling by 8
	  F_part = ((( F_part * 8)+ 50) / 100)& ((uint8_t)0x07);

   }else
   {
	   //over sampling by 16
	   F_part = ((( F_part * 16)+ 50) / 100) & ((uint8_t)0x0F);

   }

  //Place the fractional part in appropriate bit position . refer USART_BRR
  tempreg |= F_part;

  //copy the value of tempreg in to BRR register
  pUSARTx->BRR = tempreg;
}

/*********************************************************************
 * @fn      		  - USART_Init
 *
 * @brief             - This function initializes the given USART peripheral.
 *
 * @param[in]         - pUSARTHandle: pointer to the USART Handle structure
 *
 * @return            - none
 *
 * @Note              - This function configures the USART communication parameters such as baud rate,
 *                      data frame format(data bits, stop bits, parity bits), operating mode(Rx/Tx),
 *                      hardware flow control(CTS/RTS).
 *
 * @Abstraction       - This API is used by the application to initialize
 *                      and configure a USART peripheral. Internally it
 *                      programs the required USART registers based on the
 *                      user configuration present in the handle structure.
 */
void USART_Init(USART_Handle_t *pUSARTHandle)
{

	//Temporary variable
	uint32_t tempreg=0;

/******************************** Configuration of CR1******************************************/

	//Implement the code to enable the Clock for given USART peripheral
	 USART_PeriClockControl(pUSARTHandle->pUSARTx,ENABLE);

	 //Enable USART Tx and Rx engines according to the USART_Mode configuration item

	if ( pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_RX)
	{
		//Implement the code to enable the Receiver bit field
		// Receiver enabled by setting RE bit in CR1 register, transmission is disabled
		tempreg|= (1 << USART_CR1_RE);
	}else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_TX)
	{

		//Implement the code to enable the Transmitter bit field
		// Transmitter is enabled by setting TE bit in CR1 register, reception is disabled
		tempreg |= ( 1 << USART_CR1_TE );

	}else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_TXRX)
	{

		//Implement the code to enable the both Transmitter and Receiver bit fields
		// Transmission and reception enabled by setting both TE and RE bits in CR1 register
		tempreg |= ( ( 1 << USART_CR1_RE) | ( 1 << USART_CR1_TE) );
	}

    //Implement the code to configure the Word length configuration item
	// Controlls the length of data bits,
	// CR1[M] = 0 -> 8 data bits
	// CR1[M] = 1 -> 9 data bits
	tempreg |= pUSARTHandle->USART_Config.USART_WordLength << USART_CR1_M ;


    //Configuration of parity control bit fields
	if ( pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
	{
		//Implement the code to enable the parity control
		// parity enabled by setting PCE bit in CR1 register
		tempreg |= ( 1 << USART_CR1_PCE);

		//Implement the code to enable EVEN parity
		//Not required because by default EVEN parity will be selected once you enable the parity control

	}else if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD )
	{
		//Implement the code to enable the parity control
	    tempreg |= ( 1 << USART_CR1_PCE);
	    // Odd parity is selected by setting PS bit in CR1 register
	    //Implement the code to enable ODD parity
	    tempreg |= ( 1 << USART_CR1_PS);

	}

   //Program the CR1 register
	pUSARTHandle->pUSARTx->CR1 = tempreg;

/******************************** Configuration of CR2******************************************/

	tempreg=0;

	//Implement the code to configure the number of stop bits inserted during USART frame transmission
	// CR2[STOP] field:
	// 00 : 1 Stop bit    (default and most commonly used)
	// 01 : 0.5 Stop bit
	// 10 : 2 Stop bits
	// 11 : 1.5 Stop bits
	tempreg |= pUSARTHandle->USART_Config.USART_NoOfStopBits << USART_CR2_STOP;

	//Program the CR2 register
	pUSARTHandle->pUSARTx->CR2 = tempreg;

/******************************** Configuration of CR3******************************************/

	tempreg=0;

	//Configuration of USART hardware flow control
	// Hardware flow control is used for avoiding overrun errors by automatically controlling the data flow.
	// CTS (Clear To Send)   -> Input signal used by the transmitter
	//                          Transmission is allowed only when receiver assertss the line
	// RTS (Request To Send) -> Output signal used by the receiver
	//                          Indicates whether device is ready to accept more data

	if ( pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
	{
		//Implement the code to enable CTS flow control
		tempreg |= ( 1 << USART_CR3_CTSE);


	}else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
	{
		//Implement the code to enable RTS flow control
		tempreg |= ( 1 << USART_CR3_RTSE);

	}else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS_RTS)
	{
		//Implement the code to enable both CTS and RTS Flow control
		tempreg |= ( 1 << USART_CR3_CTSE);
		tempreg |= ( 1 << USART_CR3_RTSE);
	}


	pUSARTHandle->pUSARTx->CR3 = tempreg;

/******************************** Configuration of BRR(Baudrate register)******************************************/

	//Implement the code to configure the baud rate
	//We will cover this in the lecture. No action required here
	USART_SetBaudRate(pUSARTHandle->pUSARTx,pUSARTHandle->USART_Config.USART_Baud);

}




/*********************************************************************
 * @fn      		  - USART_EnableOrDisable
 *
 * @brief             - This function is used for enabling or disabling the given USART peripheral
 *
 * @param[in]         - pUSARTx: pointer to USART register definition structure
 * @param[in]         - Cmd:     Enable or Disable
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This function internally configures the UE(USART Enable)bit in the CR1 register,
 *                      hiding direct register access from the user
 *
 */
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t Cmd)
{
	if(Cmd == ENABLE)
	{
		pUSARTx->CR1 |= (1 << 13);
	}else
	{
		pUSARTx->CR1 &= ~(1 << 13);
	}

}



/*********************************************************************
 * @fn      		  - I2C_PeriClockControl
 *
 * @brief             - This function enables or disables peripheral clock for the given USART peripheral
 *
 * @param[in]         - pUSARTx: pointer to USART register definition structure
 * @param[in]         - EnorDi : Enable or Disable
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API abstracts the clock enable registers in RCC peripheral from the application layer
 *                      Internally uses APB1ENR and APB2ENR rgisters and configures corresponding bits
 */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pUSARTx == USART1)
		{
			// Enables USART1 clock by setting bit 4 in APB2ENR
			USART1_PCCK_EN();

		}else if (pUSARTx == USART2)
		{
			// Enables USART2 clock by setting bit 17 in APB1ENR
			USART2_PCCK_EN();

		}else if (pUSARTx == USART6)
		{
			// Enables USART6 clock by setting bit 5 in APB2ENR
			USART6_PCCK_EN();
		}
	}
	else
	{
		if(pUSARTx == USART1)
		{
			// Disables USART1 clock by resetting bit 4 in APB2ENR
			USART1_PCLK_DI();

		}else if (pUSARTx == USART2)
		{
			// Disables USART2 clock by resetting bit 17 in APB1ENR
			USART2_PCLK_DI();

		}else if (pUSARTx == USART6)
		{
			// Disables USART6 clock by resetting bit 5 in APB2ENR
			USART6_PCLK_DI();
		}
	}
}
/*********************************************************************
 * @fn      		  - USART_GetFlagStatus
 *
 * @brief             - This function is used to get the status of flags
 *
 * @param[in]         - pUSARTx: pointer to USART register definition structure
 * @param[in]         - StatusFlagName: name of status flag
 *
 * @return            - status of flag (1 or 0)
 *
 * @Note              - none
 *
 * @Abstraction       - This API internally reads the
 *                      corresponding bit in the USART status register (SR),
 *                      hiding direct register access from the user.
 *
 */
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint8_t StatusFlagName)
{
    if(pUSARTx->SR & StatusFlagName)
    {
    	return SET;
    }

   return RESET;
}

/*********************************************************************
 * @fn      		  - USART_SendData
 *
 * @brief             - This functions transmits a complete data buffer to the receiver.
 *
 * @param[in]         - pUSARTHandle: poinetr to USART Handle structure
 * @param[in]         - pTxBuffer : pointer to data buffer
 * @param[in]         - Len       : length of data buffer
 *
 * @return            - none
 *
 * @Note              - This function waits till transmit data regsiter is empty
 *                      before writing new data for transmission. Waits for
 *                      TC flag bfore ending transmission.
 *
 * @Abstraction       - This function internally uses USART data and status register for
 *                      data transmission, allowing the user to tansmit data without directly
 *                      accessing registers.
 */
void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len)
{

	uint16_t *pdata;

   //Loop over until "Len" number of bytes are transferred
	for(uint32_t i = 0 ; i < Len; i++)
	{
		//Implement the code to wait until TXE flag is set in the SR
		// TXE going HIGH means Transmit data register is empty and ready to accept the next byte.
		while(! USART_GetFlagStatus(pUSARTHandle->pUSARTx,USART_FLAG_TXE));

		// Transmit data based on word length and parity settings
		//
		// Effective data bits:
		// - 9 bits - when word length is 9 and parity is disabled
		// - 8 bits - when word length 1s 9 and parity is enabled
		//          - when word length is 8 and parity disabled
		// - 7 bits - when word length 1s 8 and parity is enabled
		//
		// - when parity is enabled, MSB of wordlength will be used by the parity bit
		//   reducing number of actual data bits

		//Check the USART_WordLength item for 9BIT or 8BIT in a frame
		if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
		{
			//if 9BIT load the DR with 2bytes masking  the bits other than first 9 bits
			pdata = (uint16_t*) pTxBuffer;
			pUSARTHandle->pUSARTx->DR = (*pdata & (uint16_t)0x01FF);

			//check for USART_ParityControl
			if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
			{
				//No parity is used in this transfer , so 9bits of user data will be sent
				//Implement the code to increment pTxBuffer twice
				pTxBuffer++;
				pTxBuffer++;
			}
			else
			{
				//Parity bit is used in this transfer . so 8bits of user data will be sent
				//The 9th bit will be replaced by parity bit by the hardware
				pTxBuffer++;
			}
		}
		else
		{
			//This is 8bit data transfer
			pUSARTHandle->pUSARTx->DR = (*pTxBuffer  & (uint8_t)0xFF);

			//Implement the code to increment the buffer address
			pTxBuffer++;
		}
	}

	//Implement the code to wait till TC flag is set in the SR
	// wait till all data frames are  shifted out of the shift register before ending communication.
	while( ! USART_GetFlagStatus(pUSARTHandle->pUSARTx,USART_FLAG_TC));
}


/*********************************************************************
 * @fn      		  - USART_ReceiveData
 *
 * @brief             - This function receives data into a receive buffer from the transmitter.
 *
 * @param[in]         - pUSARTHandle : pointer to USART register definition structure
 * @param[in]         - pRxBuffer    : pointer to receiver buffer
 * @param[in]         - Len          : length of data to be received
 *
 * @return            - none
 *
 * @Note              - This function receives data by waiting for the RXNE flag to
 *                      indicate that data has been received
 *                      and reads the data register based on wordlength and parity settings
 *
 *@Abstraction        - This function abstracts the USART status and data register
 *                      from the user, allowing the application to receive data
 *                      into a buffer without accessing registers
 *
 */

void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
   //Loop over until "Len" number of bytes are transferred
	for(uint32_t i = 0 ; i < Len; i++)
	{
		//Implement the code to wait until RXNE flag is set in the SR
		while(! USART_GetFlagStatus(pUSARTHandle->pUSARTx,USART_FLAG_RXNE));


		// Receive data based on word length and parity settings
		//
		// Effective data bits:
		// - 9 bits - when word length is 9 and parity is disabled
		// - 8 bits - when word length 1s 9 and parity is enabled
		//          - when word length is 8 and parity disabled
		// - 7 bits - when word length 1s 8 and parity is enabled
		//
		// - when parity is enabled, MSB of wordlength will be used by the parity bit
		//   reducing number of actual data bits

		//Check the USART_WordLength to decide whether we are going to receive 9bit of data in a frame or 8 bit
		if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
		{
			//We are going to receive 9bit data in a frame

			//Now, check are we using USART_ParityControl control or not
			if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
			{
				//No parity is used , so all 9bits will be of user data

				//read only first 9 bits so mask the DR with 0x01FF
				*((uint16_t*) pRxBuffer) = (pUSARTHandle->pUSARTx->DR  & (uint16_t)0x01FF);

				//Now increment the pRxBuffer two times
				pRxBuffer++;
				pRxBuffer++;
			}
			else
			{
				//Parity is used, so 8bits will be of user data and 1 bit is parity
				 *pRxBuffer = (pUSARTHandle->pUSARTx->DR  & (uint8_t)0xFF);
				 pRxBuffer++;
			}
		}
		else
		{
			//We are going to receive 8bit data in a frame

			//Now, check are we using USART_ParityControl control or not
			if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
			{
				//No parity is used , so all 8bits will be of user data

				//read 8 bits from DR
				 *pRxBuffer = (uint8_t) (pUSARTHandle->pUSARTx->DR  & (uint8_t)0xFF);
			}

			else
			{
				//Parity is used, so , 7 bits will be of user data and 1 bit is parity

				//read only 7 bits , hence mask the DR with 0X7F
				 *pRxBuffer = (uint8_t) (pUSARTHandle->pUSARTx->DR  & (uint8_t)0x7F);

			}

			//Now , increment the pRxBuffer
			pRxBuffer++;
		}
	}

}

/*********************************************************************
 * @fn      		  - USART_SendDataIT
 *
 * @brief             - This function starts interrupt-driven USART data transmission.
 *
 * @param[in]         - pUSARTHandle: pointer to USART Handle structure
 * @param[in]         - pTxBuffer   : pointer to transmit buffer
 * @param[in]         - Len         : length of data to be transmitted
 *
 * @return            - txstate     : data transmission state before the function call.
 *
 * @Note              - This function only initiates a data transmission,
 *                      it stores the data buffer information, sets the txstate as busy,
 *                      and enables TXE, TC interrupts. Actual data transfer is handled in the IRQ Handler.
 *
 * @Abstraction       - The function hides the register-level interrupt configurations
 *                      and internal state management from the application.
 *
 */
uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer, uint32_t Len)
{
	// read the current state of transmission
	uint8_t txstate = pUSARTHandle->TxBusyState;

	// initiate next transmission only when the USART is not busy in transmitting data
	if(txstate != USART_BUSY_IN_TX)
	{
		// store buffer details to the handle structure and set the state as busy
		pUSARTHandle->TxLen       = Len;
		pUSARTHandle->pTxBuffer   = pTxBuffer;
		pUSARTHandle->TxBusyState = USART_BUSY_IN_TX;

		//Implement the code to enable interrupt for TXE
		pUSARTHandle->pUSARTx->CR1 |= ( 1 << USART_CR1_TXEIE);

		//Implement the code to enable interrupt for TC
		pUSARTHandle->pUSARTx->CR1 |= ( 1 << USART_CR1_TCIE);


	}

	return txstate;
}


/*********************************************************************
 * @fn      		  - USART_ReceiveDataIT
 *
 * @brief             - This function initiates interrupt-driven USART data reception.
 *
 * @param[in]         - pUSARTHandle: pointer to USART Handle structure
 * @param[in]         - pRxBuffer   : pointer to receive buffer
 * @param[in]         - Len         : length of data to be received
 *
 * @return            - rxstate     : data reception state before the function call
 *
 * @Note              - This function starts a data reception in an interrupt-driven manner,
 *                      it sets the pointer to receive buffer as handle buffer,
 *                      stores the length of data to be received in the handler,
 *                      sets the reception state as busy and enables the RXNE interrupt
 *                      Actual data reception is performed by the IRQ handler.
 *
 * @Abstraction       - The function hides the register-level interrupt configurations
 *                      and internal state management from the application.
 */
uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer, uint32_t Len)
{
	// read the current state of reception
	uint8_t rxstate = pUSARTHandle->RxBusyState;

	// start reception only when the USART is not busy
	if(rxstate != USART_BUSY_IN_RX)
	{
		// stores buffer details in handle and set state as busy
		pUSARTHandle->RxLen = Len;
		pUSARTHandle->pRxBuffer = pRxBuffer;
		pUSARTHandle->RxBusyState = USART_BUSY_IN_RX;

		// perform a dummy read to clear RXNE flag
		(void)pUSARTHandle->pUSARTx->DR;

		//Implement the code to enable interrupt for RXNE
		pUSARTHandle->pUSARTx->CR1 |= ( 1 << USART_CR1_RXNEIE);

	}

	return rxstate;
}


/*********************************************************************
 * @fn      		  - USART_ClearFlag
 *
 * @brief             - This function is used to clear a specific flag in the status register.
 *
 * @param[in]         - pUSARTx       : pointer to USART regsiter definition structure
 * @param[in]         - StatusFlagName: name of flag to be cleared.
 *
 * @return            - none
 *
 * @Note              - Applicable to only USART_CTS_FLAG , USART_LBD_FLAG
 *                      USART_TC_FLAGs
 *
 * @abstraction       - This clears the corresponding flag bit in the status register,
 *                      abstracting direct register acces from the application.
 */

void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t StatusFlagName)
{
	pUSARTx->SR &= ~( StatusFlagName);

}

/*********************************************************************
 * @fn      		  - USART_IRQInterruptConfig
 *
 * @brief             - This function is used to enable or disable an interrupt request.
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
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
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
			*NVIC_ISER3 |= ( 1 << (IRQNumber % 64) );
		}
	}else // IRQ Disable
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
		}else if(IRQNumber >= 32 && IRQNumber < 64 )
		{
			//program ICER1 register
			*NVIC_ICER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96 )
		{
			//program ICER2 register
			*NVIC_ICER3 |= ( 1 << (IRQNumber % 64) );
		}
	}

}


/*********************************************************************
 * @fn      		  - USART_IRQPriorityConfig
 *
 * @brief             - This function configures the priority for a USART IRQ
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
 * @Abstraction       - This API is used by the application to set the priority of a specific IRQ
 *                      Internally modifies the corresponding bit fields in the IPRx register,
 *                      hiding direct register access from the user.
 *
 */
void USART_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority)
{
	//1. first lets find out the ipr register
	    //
		// - Each IPR register handles 4 IRQ lines
		// - An 8 bit priority feild is allocated for each IRQ
		// - STM32F4 uses upper 4 bits of the field to set the priority
		// Examples:
		// - IRQ0  - IPR[0][7:0]
		// - IRQ1  - IPR[0][15:8]
		// - IRQ2  - IPR[0][23:16]
		// - IRQ3  - IPR[0][24:31]
		// - IRQ4  - IPR[1][7:0]

	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section  = IRQNumber %4 ;

	uint8_t shift_amount = ( 8 * iprx_section) + ( 8 - NO_PR_BITS_IMPLEMENTED) ;

	*(  NVIC_PR_BASE_ADDR + iprx ) |=  ( IRQPriority << shift_amount );

}

/*********************************************************************
 * @fn      		  - USART_IRQHandler
 *
 * @brief             - This function handles USART interrupt events for data
 *                      transmission, reception, and error conditions.
 *
 * @param[in]         - pUSARTHandle: pointer to USART Handle structure
 *
 * @return            - none
 *
 * @Note              - This function manages all interrupt sources including TXE,TC,RXNE,
 *                      all error flags (PE, NE, ORE, FE), CTS and IDLE events.
 *                      Performs
 *                            - data transmission using TXE,TC interrupts
 *                            - reception using RXNE interrupt
 *                            - error detection
 *                      It also notifies the application layer by calling
 *                      USART_ApplicationEventCallback() for significant
 *                      events such as TX complete, RX complete, and errors.
 *
 * @Abstraction       - Acts as core of interrupt-driven USART driver
 *                     it hides internal RX/TX state management, ISR handling
 *                     and notifies only high level events to the application.
 *
 */
void USART_IRQHandling(USART_Handle_t *pUSARTHandle)
{

	uint32_t temp1 , temp2, temp3;

	uint16_t *pdata;

/*************************Check for TC flag ********************************************/

    //Implement the code to check the state of TC bit in the SR
	temp1 = pUSARTHandle->pUSARTx->SR & ( 1 << USART_SR_TC);

	 //Implement the code to check the state of TCEIE bit
	temp2 = pUSARTHandle->pUSARTx->CR1 & ( 1 << USART_CR1_TCIE);

	if(temp1 && temp2 )
	{
		//this interrupt is because of TC

		//close transmission and call application callback if TxLen is zero
		if ( pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
		{
			//Check the TxLen . If it is zero then close the data transmission
			if(! pUSARTHandle->TxLen )
			{
				//Implement the code to clear the TC flag
				pUSARTHandle->pUSARTx->SR &= ~( 1 << USART_SR_TC);

				//Implement the code to clear the TCIE control bit
				pUSARTHandle->pUSARTx->CR1 &= ~( 1 << USART_CR1_TCIE);

				//Reset the application state
				pUSARTHandle->TxBusyState = USART_READY;

				//Reset Buffer address to NULL
				pUSARTHandle->pTxBuffer = NULL;

				//Reset the length to zero
				pUSARTHandle->TxLen = 0;

				//Call the application call back with event USART_EVENT_TX_CMPLT
				USART_ApplicationEventCallback(pUSARTHandle,USART_EVENT_TX_CMPLT);
			}
		}
	}

/*************************Check for TXE flag ********************************************/

	//Implement the code to check the state of TXE bit in the SR
	temp1 = pUSARTHandle->pUSARTx->SR & ( 1 << USART_SR_TXE);

	//Implement the code to check the state of TXEIE bit in CR1
	temp2 = pUSARTHandle->pUSARTx->CR1 & ( 1 << USART_CR1_TXEIE);


	if(temp1 && temp2 )
	{
		//this interrupt is because of TXE

		if(pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
		{
			//Keep sending data until Txlen reaches to zero
			if(pUSARTHandle->TxLen > 0)
			{
				//Check the USART_WordLength item for 9BIT or 8BIT in a frame
				if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
				{
					//if 9BIT load the DR with 2bytes masking  the bits other than first 9 bits
					pdata = (uint16_t*) pUSARTHandle->pTxBuffer;
					pUSARTHandle->pUSARTx->DR = (*pdata & (uint16_t)0x01FF);

					//check for USART_ParityControl
					if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
					{
						//No parity is used in this transfer , so 9bits of user data will be sent
						//Implement the code to increment pTxBuffer twice
						pUSARTHandle->pTxBuffer++;
						pUSARTHandle->pTxBuffer++;
						pUSARTHandle->TxLen-=2;
					}
					else
					{
						//Parity bit is used in this transfer . so 8bits of user data will be sent
						//The 9th bit will be replaced by parity bit by the hardware
						pUSARTHandle->pTxBuffer++;
						pUSARTHandle->TxLen-=1;
					}
				}
				else
				{
					//This is 8bit data transfer
					pUSARTHandle->pUSARTx->DR = (*pUSARTHandle->pTxBuffer  & (uint8_t)0xFF);

					//Implement the code to increment the buffer address
					pUSARTHandle->pTxBuffer++;
					pUSARTHandle->TxLen-=1;
				}

			}
			if (pUSARTHandle->TxLen == 0 )
			{
				//TxLen is zero
				//Implement the code to clear the TXEIE bit (disable interrupt for TXE flag )
				pUSARTHandle->pUSARTx->CR1 &= ~( 1 << USART_CR1_TXEIE);
			}
		}
	}

/*************************Check for RXNE flag ********************************************/

	temp1 = pUSARTHandle->pUSARTx->SR & ( 1 << USART_SR_RXNE);
	temp2 = pUSARTHandle->pUSARTx->CR1 & ( 1 << USART_CR1_RXNEIE);


	if( temp1 && temp2 )
	{
		//this interrupt is because of rxne
		if(pUSARTHandle->RxBusyState == USART_BUSY_IN_RX)
		{
			if(pUSARTHandle->RxLen > 0)
			{
				//Check the USART_WordLength to decide whether we are going to receive 9bit of data in a frame or 8 bit
				if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
				{
					//We are going to receive 9bit data in a frame

					//Now, check are we using USART_ParityControl control or not
					if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
					{
						//No parity is used , so all 9bits will be of user data

						//read only first 9 bits so mask the DR with 0x01FF
						*((uint16_t*) pUSARTHandle->pRxBuffer) = (pUSARTHandle->pUSARTx->DR  & (uint16_t)0x01FF);

						//Now increment the pRxBuffer two times
						pUSARTHandle->pRxBuffer++;
						pUSARTHandle->pRxBuffer++;
						pUSARTHandle->RxLen-=2;
					}
					else
					{
						//Parity is used, so 8bits will be of user data and 1 bit is parity
						 *pUSARTHandle->pRxBuffer = (pUSARTHandle->pUSARTx->DR  & (uint8_t)0xFF);
						 pUSARTHandle->pRxBuffer++;
						 pUSARTHandle->RxLen-=1;
					}
				}
				else
				{
					//We are going to receive 8bit data in a frame

					//Now, check are we using USART_ParityControl control or not
					if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
					{
						//No parity is used , so all 8bits will be of user data

						//read 8 bits from DR
						 *pUSARTHandle->pRxBuffer = (uint8_t) (pUSARTHandle->pUSARTx->DR  & (uint8_t)0xFF);

					}

					else
					{
						//Parity is used, so , 7 bits will be of user data and 1 bit is parity

						//read only 7 bits , hence mask the DR with 0X7F
						 *pUSARTHandle->pRxBuffer = (uint8_t) (pUSARTHandle->pUSARTx->DR  & (uint8_t)0x7F);

					}

					//Now , increment the pRxBuffer
					pUSARTHandle->pRxBuffer++;
					 pUSARTHandle->RxLen-=1;
				}


			}//if of >0

			if(! pUSARTHandle->RxLen)
			{
				//disable the rxne
				pUSARTHandle->pUSARTx->CR1 &= ~( 1 << USART_CR1_RXNEIE );
				pUSARTHandle->RxBusyState = USART_READY;
				USART_ApplicationEventCallback(pUSARTHandle,USART_EVENT_RX_CMPLT);
			}
		}
	}


/*************************Check for CTS flag ********************************************/
//Note : CTS feature is not applicable for UART4 and UART5

	//Implement the code to check the status of CTS bit in the SR
	temp1 = pUSARTHandle->pUSARTx->SR & ( 1 << USART_SR_CTS);

	//Implement the code to check the state of CTSE bit in CR1
	temp2 = pUSARTHandle->pUSARTx->CR3 & ( 1 << USART_CR3_CTSE);

	//Implement the code to check the state of CTSIE bit in CR3 (This bit is not available for UART4 & UART5.)
	temp3 = pUSARTHandle->pUSARTx->CR3 & ( 1 << USART_CR3_CTSIE);


	if(temp1  && temp2 && temp3)
	{
		//Implement the code to clear the CTS flag in SR
		pUSARTHandle->pUSARTx->SR &=  ~( 1 << USART_SR_CTS);

		//this interrupt is because of cts
		USART_ApplicationEventCallback(pUSARTHandle,USART_EVENT_CTS);
	}

/*************************Check for IDLE detection flag ********************************************/

	//Implement the code to check the status of IDLE flag bit in the SR
	temp1 = pUSARTHandle->pUSARTx->SR & ( 1 << USART_SR_IDLE);

	//Implement the code to check the state of IDLEIE bit in CR1
	temp2 = pUSARTHandle->pUSARTx->CR1 & ( 1 << USART_CR1_IDLEIE);


	if(temp1 && temp2)
	{
		//Implement the code to clear the IDLE flag. Refer to the RM to understand the clear sequence
		temp1 = pUSARTHandle->pUSARTx->SR &= ~( 1 << USART_SR_IDLE);

		//this interrupt is because of idle
		USART_ApplicationEventCallback(pUSARTHandle,USART_EVENT_IDLE);
	}

/*************************Check for Overrun detection flag ********************************************/

	//Implement the code to check the status of ORE flag  in the SR
	temp1 = pUSARTHandle->pUSARTx->SR & USART_SR_ORE;

	//Implement the code to check the status of RXNEIE  bit in the CR1
	temp2 = pUSARTHandle->pUSARTx->CR1 & USART_CR1_RXNEIE;


	if(temp1  && temp2 )
	{
		//Need not to clear the ORE flag here, instead give an api for the application to clear the ORE flag .
		USART_ClearOREFlag(pUSARTHandle->pUSARTx);

		//this interrupt is because of Overrun error
		USART_ApplicationEventCallback(pUSARTHandle,USART_ERR_ORE);
	}



/*************************Check for Error Flag ********************************************/

//Noise Flag, Overrun error and Framing Error in multibuffer communication
//We dont discuss multibuffer communication in this course. please refer to the RM
//The blow code will get executed in only if multibuffer mode is used.

	temp2 =  pUSARTHandle->pUSARTx->CR3 & ( 1 << USART_CR3_EIE) ;

	if(temp2 )
	{
		temp1 = pUSARTHandle->pUSARTx->SR;
		if(temp1 & ( 1 << USART_SR_FE))
		{
			/*
				This bit is set by hardware when a de-synchronization, excessive noise or a break character
				is detected. It is cleared by a software sequence (an read to the USART_SR register
				followed by a read to the USART_DR register).
			*/
			USART_ApplicationEventCallback(pUSARTHandle,USART_ERR_FE);
		}

		if(temp1 & ( 1 << USART_SR_NE) )
		{
			/*
				This bit is set by hardware when noise is detected on a received frame. It is cleared by a
				software sequence (an read to the USART_SR register followed by a read to the
				USART_DR register).
			*/
			USART_ApplicationEventCallback(pUSARTHandle,USART_ERR_NE);
		}

		if(temp1 & ( 1 << USART_SR_ORE) )
		{
			USART_ApplicationEventCallback(pUSARTHandle,USART_ERR_ORE);
		}
	}


}
void USART_ClearOREFlag(USART_RegDef_t* pUSARTx)
{
	(void)pUSARTx->SR;
	(void)pUSARTx->DR;

}


/*********************************************************************
 * @fn      		  - USART_ApplicationEventCallback
 *
 * @brief             - This is a weak call back function called by the IRQ Handler to notify
 *                      the application layer about USART events.
 *
 * @param[in]         - pUSARTHandle: pointer to USART Handle structure
 * @param[in]         - event       : Event generated
 *
 * @return            - none
 *
 * @Note              - This is a weak function, the user
 *                      can redefine this function in the application layer to
 *                      implement custom logic for USART events.
 *                      This function is called from an ISR, so it must be short and non-blocking
 *
 * @Abstraction       - Provides an interface between USART driver and application,
 *                      allowing the application to handle  high level USART events and
 *                      implements custom logic withput modifying the driver.
 */
__weak void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle,uint8_t event)
{

}
