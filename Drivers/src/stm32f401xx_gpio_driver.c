/*
 * stm32f446xx_gpio_driver.c
 *
 * GPIO driver source file for STM32F446xx MCU
 */

#include "stm32f401xx_gpio_driver.h"


/*********************************************************************
 * @fn                - GPIO_PeriClockControl
 *
 * @brief             - This function enables or disables peripheral clock for the given GPIO port
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 * @param[in]         - EnorDi: ENABLE or DISABLE macros
 *
 * @return            - none
 *
 * @Note              - STM32F401RE provides only 6 GPIO ports (A,B,C,D,E,H)
 *
 * @Abstraction       - This API abstracts the clock enable registers in RCC peripheral from the application layer
 *                    - Internally uses AHB1ENR rgister and configures corresponding bits
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pGPIOx == GPIOA)
        {
        	// Enables GPIOA clock by setting bit 0 in AHB1ENR
            GPIOA_PCLK_EN();
        }
        else if (pGPIOx == GPIOB)
        {
        	// Enables GPIOA clock by setting bit 1 in AHB1ENR
            GPIOB_PCLK_EN();
        }
        else if (pGPIOx == GPIOC)
        {
        	// Enables GPIOA clock by setting bit 2 in AHB1ENR
            GPIOC_PCLK_EN();
        }
        else if (pGPIOx == GPIOD)
        {
        	// Enables GPIOA clock by setting bit 3 in AHB1ENR
            GPIOD_PCLK_EN();
        }
        else if (pGPIOx == GPIOE)
        {
        	// Enables GPIOA clock by setting bit 4 in AHB1ENR
            GPIOE_PCLK_EN();
        }
        else if (pGPIOx == GPIOH)
        {
        	// Enables GPIOA clock by setting bit 7 in AHB1ENR
            GPIOH_PCLK_EN();
        }
    }
    else
    {
        if (pGPIOx == GPIOA)
        {
        	// Enables GPIOA clock by resetting bit 0 in AHB1ENR
            GPIOA_PCLK_DI();
        }
        else if (pGPIOx == GPIOB)
        {
        	// Enables GPIOA clock by resetting bit 1 in AHB1ENR
            GPIOB_PCLK_DI();
        }
        else if (pGPIOx == GPIOC)
        {
        	// Enables GPIOA clock by resetting bit 2 in AHB1ENR
            GPIOC_PCLK_DI();
        }
        else if (pGPIOx == GPIOD)
        {
        	// Enables GPIOA clock by resetting bit 3 in AHB1ENR
            GPIOD_PCLK_DI();
        }
        else if (pGPIOx == GPIOE)
        {
        	// Enables GPIOA clock by resetting bit 4 in AHB1ENR
            GPIOE_PCLK_DI();
        }
        else if (pGPIOx == GPIOH)
        {
        	// Enables GPIOA clock by resetting bit 7 in AHB1ENR
            GPIOH_PCLK_DI();
        }
    }
}


/*********************************************************************
 * @fn                - GPIO_Init
 *
 * @brief             - This function initializes the given GPIO port pin
 *
 * @param[in]         - pGPIOHandle: pointer to GPIO handle structure
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API hides direct GPIO register configuration from the application
 *                    - It Internally uses settings provided by GPIO_Handle_t structure to configure
 *                      MODER, OTYPER, OSPEEDER, PUPDR, AFR registers.
 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
    uint32_t temp = 0;

    // Enable the peripheral clock
    GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);

    // 1. Configure the mode of GPIO pin
    if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ANALOG)
    {
        // Non-interrupt mode
    	// MODER register uses 2 bits to control the mode of each pins
    	// - 00 -> input mode
    	// - 01 -> output mode
    	// - 10 -> alternate function mode
    	// - 11 -> analog mode
    	// - Pin 0  -> MODER[1:0]
    	// - Pin 1  -> MODER[3:2]
    	// - Pin n  -> MODER[(2n+1):2n]
        temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
        pGPIOHandle->pGPIOx->MODER &= ~(0x3 << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber)); // Clearing
        pGPIOHandle->pGPIOx->MODER |= temp; // Setting
    }
    else
    {
        // Interrupt mode
    	//
    	// GPIO interrupts are handles by using EXTI and SYSCFG peripherals
    	// - EXTI   : Detects interrupt events on GPIO pins
    	// - SYSCFG : Maps a GPIO port to an EXTI line
    	//
    	// EXTI registers:
    	// - FTSR & RTSR       - decides the interrupt mode (interrupt on falling edge or rising edge)
    	// - IMR               - enables or disables the inetrrupt
    	//
    	// SYSCFG register:
    	// - EXTICR            - selects which GPIO port is connected to an EXTI line


        if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT)
        {
            // 1. Configure the FTSR (Falling Trigger Selection Register)
            EXTI->FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            // Clear the corresponding RTSR bit
            EXTI->RTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT)
        {
            // 1. Configure the RTSR (Rising Trigger Selection Register)
            EXTI->RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            // Clear the corresponding FTSR bit
            EXTI->FTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT)
        {
            // 1. Configure both FTSR and RTSR
            EXTI->RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }

        // 2. Configure the GPIO port selection in SYSCFG_EXTICR

        // Determine the EXTICR register
        // - each EXTICR register controls 4 EXTI lines
        uint8_t temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 4;

        // Determine the correct bit position inside the selected regsiter
        // - each EXTI line uses 4 bits
        uint8_t temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 4;

        // Select the GPIO port code for given GPIO
        // - eg: GPIOA -> 0000, GPIOB -> 0001
        uint8_t portcode = GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);

        // Enable clock for sysconfig peripheral before accessing its registers
        SYSCFG_PCLK_EN();
        SYSCFG->EXTICR[temp1] = portcode << (temp2 * 4);

        // 3. Enable the EXTI interrupt delivery using IMR
        EXTI->IMR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

        // Abstraction:
        // The user selects an interrupt mode through GPIO_PinMode.
        // The driver internally configures EXTI registers,
        // maps the GPIO port to the correct EXTI line using SYSCFG,
        // and enables interrupt through the IMR register
        // enabling IRQ in NVIC is done seperately.
    }

    // 2. Configure the speed

    // Speed is configured using OSPEEDER register
    // Each pin is controlled using 2 bits
    // - 00 -> low speed
    // - 01 -> medium speed
    // - 10 -> high speed
    // - 11 -> very high speed
    // - eg: pin0 -> OSPEEDR[1:0]
    // -     pin1 -> OSPEEDR[3:2]
    temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
    pGPIOHandle->pGPIOx->OSPEEDR &= ~(0x3 << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber)); // Clearing
    pGPIOHandle->pGPIOx->OSPEEDR |= temp;

    // 3. Configure the pull-up/pull-down settings

    // Pull-up/pull-down is set using PUPDR register
    // Each pin is controlled using 2 bits
    // - 00 -> no pull-up or pull-down
    // - 01 -> pull-up
    // - 10 -> pull-down
    // - 11 -> reserved
    // - eg: pin0 -> PUPDR[1:0]
    // -     pin1 -> PUPDR[3:2]
    temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
    pGPIOHandle->pGPIOx->PUPDR &= ~(0x3 << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber)); // Clearing
    pGPIOHandle->pGPIOx->PUPDR |= temp;

    // 4. Configure the output type

    // Output type is set using OTYPER register
    // Each pin is controlled using 1 bit
    // - 0 -> push-pull type output
    // - 1 -> open-drain type output
    // - eg: pin0 -> OTYPER[0]
    // -     pin1 -> OTYPER[1]
    temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinOPType << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->OTYPER &= ~(0x1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber); // Clearing
    pGPIOHandle->pGPIOx->OTYPER |= temp;

    // 5. Configure the alternate functionality

    // Alternate function is set using 2 AFR registers
    //
    // Each register handles 8 pins
    // - AFR[0] -> pin0-pin7
    // - AFR[1] -> pin8-pin15
    //
    // Each pin is controlled using 4 bits
    // - eg: pin0 -> AFR[0][3:0]
    // -     pin7 -> AFR[0][31:28]
    // -     pin9 -> AFR[1][7:4]
    //
    // Example alternate functions:
    // AF4 -> I2C
    // AF5 -> SPI
    // AF7 -> USART
    if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_ALTFN)
    {
        // Configure the alternate function registers
        uint8_t temp1, temp2;

        temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 8;
        temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 8;
        pGPIOHandle->pGPIOx->AFR[temp1] &= ~(0xF << (4 * temp2)); // Clearing
        pGPIOHandle->pGPIOx->AFR[temp1] |= (pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode << (4 * temp2));
    }
}


/*********************************************************************
 * @fn                - GPIO_DeInit
 *
 * @brief             - This function resets the given GPIO port to its default state
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API resets a GPIO port using appropriate reset register and bits in the RCC peripheral
 *                    - hides the GPIO reset mechanism from application layer
 */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx)
{
	// Reset mechansim:
	//
	// GPIO peripheral is reset using RCC->AHB1RSTR.
	//
	// Reset sequence:
	// 1. Set the corresponding bit in AHB1RSTR.
	//    This forces the peripheral into its reset state.
	// 2. Clear the bit.
	//    This releases the peripheral from reset.
	//
	// After reset, all GPIO registers return to their default values.
    if (pGPIOx == GPIOA)
    {
    	// reset GPIOA using GPIOARST bit (bit0)
        GPIOA_REG_RESET();
    }
    else if (pGPIOx == GPIOB)
    {
    	// reset GPIOA using GPIOBRST bit (bit1)
        GPIOB_REG_RESET();
    }
    else if (pGPIOx == GPIOC)
    {
    	// reset GPIOA using GPIOCRST bit (bit2)
        GPIOC_REG_RESET();
    }
    else if (pGPIOx == GPIOD)
    {
    	// reset GPIOD using GPIOARST bit (bit3)
        GPIOD_REG_RESET();
    }
    else if (pGPIOx == GPIOE)
    {
    	// reset GPIOE using GPIOARST bit (bit4)
        GPIOE_REG_RESET();
    }
    else if (pGPIOx == GPIOH)
    {
    	// reset GPIOH using GPIOHRST bit (bit7)
        GPIOH_REG_RESET();
    }
}


/*********************************************************************
 * @fn                - GPIO_ReadFromInputPin
 *
 * @brief             - This function reads from a specific GPIO pin
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 * @param[in]         - PinNumber: GPIO pin number
 *
 * @return            - 0 or 1
 *
 * @Note              - none
 *
 * @Abstraction       - This API is used to read the state of a GPIO pin by reading
 *                    - corresponding bit in the IDR register
 *
 */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    uint8_t value;

    // IDR (Input Data Register):
    // - Each bit corresponds to one GPIO pin.
    // - IDR[0] -> Pin 0
    // - IDR[1] -> Pin 1
    value = (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x00000001);

    return value;
}


/*********************************************************************
 * @fn                - GPIO_ReadFromInputPort
 *
 * @brief             - This function reads from a GPIO port
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 *
 * @return            - 16-bit value of the port
 *
 * @Note              - none
 *
 * @Abstraction       - The application reads the complete GPIO port
 *                      state using a simple API call. The driver
 *                      internally accesses the GPIO Input Data
 *                      Register (IDR) and returns the pin states.
 */
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx)
{
    uint16_t value;

    value = (uint16_t)pGPIOx->IDR;

    return value;
}


/*********************************************************************
 * @fn                - GPIO_WriteToOutputPin
 *
 * @brief             - This function writes to a specific GPIO pin
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 * @param[in]         - PinNumber: GPIO pin number
 * @param[in]         - Value: GPIO_PIN_SET or GPIO_PIN_RESET
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API is used by the application to control the output state of a pin
 *                    - Modifies the appropriate bit it in the ODR (Output Data Register)
 *                      and hides direct register access from the application.
 */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value)
{
    if (Value == GPIO_PIN_SET)
    {
        // Write 1 to the output data register at the bit field corresponding to the pin number
        pGPIOx->ODR |= (1 << PinNumber);
    }
    else
    {
        // Write 0
        pGPIOx->ODR &= ~(1 << PinNumber);
    }
}


/*********************************************************************
 * @fn                - GPIO_WriteToOutputPort
 *
 * @brief             - This function writes to a GPIO port
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 * @param[in]         - Value: 16-bit value to write
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API is used by the application to control the output state of an entire port
 *                      The driver internally writes the provided value to the GPIO Output Data Register (IDR),
 *                      hiding direct register access from the user.
 */
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value)
{
    pGPIOx->ODR = Value;
}


/*********************************************************************
 * @fn                - GPIO_ToggleOutputPin
 *
 * @brief             - This function toggles a specific GPIO pin
 *
 * @param[in]         - pGPIOx: base address of the GPIO peripheral
 * @param[in]         - PinNumber: GPIO pin number
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API used by the application to toggle the output state of a pin
 *                      internally toggles the corresponding bit in the Output Data Register and
 *                      hides direct register access from user
 */
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    pGPIOx->ODR ^= (1 << PinNumber);
}


/*********************************************************************
 * @fn                - GPIO_IRQInterruptConfig
 *
 * @brief             - This function configures the IRQ for a GPIO pin
 *
 * @param[in]         - IRQNumber: IRQ number
 * @param[in]         - EnorDi: ENABLE or DISABLE macros
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API is used by the application to enable or disable
 *                      a specific interrupt request (IRQ) by directly manipulating
 *                      ISER and ICER registers.
 */
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
    	// Interrupt Set Enable Resgister (ISER) is used for enabling an IRQ
    	// - each registers handle 32 IRQs
    	// - ISER[0] = IRQ 0-IRQ 31
    	// - ISER[1] = IRQ 32-IRQ 63
    	// - ISER[2] = IRQ 64-IRQ 95
    	// - each bit corresponds to one interrupt line

        if (IRQNumber <= 31)
        {
            // Program ISER0 register
            *NVIC_ISER0 |= (1 << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ISER1 register (32 to 63)
            *NVIC_ISER1 |= (1 << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            // Program ISER2 register (64 to 95)
            *NVIC_ISER2 |= (1 << (IRQNumber % 64));
        }
    }
    else// Disable an IRQ
    {
    	// Interrupt Clear Enable Resgister (ICER) is used for disabling an IRQ
    	// - each registers handle 32 IRQs
    	// - ICER[0] = IRQ 0-IRQ 31
    	// - ICER[1] = IRQ 32-IRQ 63
        // - ICER[2] = IRQ 64-IRQ 95
    	// - each bit corresponds to one interrupt line

        if (IRQNumber <= 31)
        {
            // Program ICER0 register
            *NVIC_ICER0 |= (1 << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ICER1 register
            *NVIC_ICER1 |= (1 << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            // Program ICER2 register
            *NVIC_ICER2 |= (1 << (IRQNumber % 64));
        }
    }
}


/*********************************************************************
 * @fn                - GPIO_IRQPriorityConfig
 *
 * @brief             - This function configures the priority for a GPIO IRQ
 *
 * @param[in]         - IRQNumber: IRQ number
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
 */
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
    // 1. First find out the IPR register
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
    uint8_t iprx_section = IRQNumber % 4;

    uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

    *(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}


/*********************************************************************
 * @fn                - GPIO_IRQHandling
 *
 * @brief             - This function handles the GPIO IRQ
 *
 * @param[in]         - PinNumber: GPIO pin number
 *
 * @return            - none
 *
 * @Note              - none
 *
 * @Abstraction       - This API abstracts direct register access from the user and provides a safe method
 *                      to clear the interrupt flag.
 */
void GPIO_IRQHandling(uint8_t PinNumber)
{
    // Clear the EXTI PR register corresponding to the pin number
    if (EXTI->PR & (1 << PinNumber))
    {
        // Clear by writing 1
        EXTI->PR |= (1 << PinNumber);
    }
}

