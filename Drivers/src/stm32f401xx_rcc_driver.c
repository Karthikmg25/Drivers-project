/*
 * stm32f446xx_rcc_driver.c
 * RCC Driver for STM32F446xx MCU
 * Author: Neeraj
 */

#include <stm32f401xx_rcc_driver.h>

// Store AHB prescaler values in an array
uint16_t AHB_PreScaler[8] = {2,4,8,16,64,128,256,512};

// Store APB prescaler values in an array
uint8_t APB1_PreScaler[4] = { 2, 4 , 8, 16};



/*********************************************************************
 * @fn      		  - RCC_GetPCLK2Value
 *
 * @brief             - This function provides the frequency of APB1 clock
 *
 * @param[in]         - none
 *
 * @return            - pclk1 - APB1 frequency
 *
 * @Note              - Assumes HSI = 16 MHz and HSE = 8 MHz.
 *
 * @Abstraction       - This function abstracts clock frequency calculations and
 *                      register accessing from the application, allowing the application
 *                      to read the APB1 clock frequency through a simple API call.
 *
 */
uint32_t RCC_GetPCLK1Value(void)
{
	// 1) Find the system clock by checking SWS bits in CFGR register
	//     - SWS[1:0] = 00 -> HSI
	//     - SWS[1:0] = 01 -> HSE
	//     - SWS[1:0] = 10 -> PLL
	//
	// 2) Find AHB prescaler by checking HPRE bits in CFGR
	//     - HPRE[3:0] = 0xxx ->  prescaler 1 (no division)
	//     - HPRE[3:0] = 1000 ->  prescaler 2
	//     - HPRE[3:0] = 1001 ->  prescaler 4
	//     - HPRE[3:0] = 1010 ->  prescaler 8
	//     - HPRE[3:0] = 1011 ->  prescaler 16
	//     - HPRE[3:0] = 1100 ->  prescaler 64
	//     - HPRE[3:0] = 1101 ->  prescaler 128
	//     - HPRE[3:0] = 1110 ->  prescaler 256
	//     - HPRE[3:0] = 1111 ->  prescaler 5121
	//
	// 3) AHB frequency = system clock frequency/ AHB prescaler
	//
	// 4) Fing APB1 prescaler by checing PPRE1 bits in CFGR
	//     - PPRE1[2:0] = 0xx ->  prescaler 1 (no division)
	//     - PPRE1[2:0] = 100 ->  prescaler 2
	//     - PPRE1[2:0] = 101 ->  prescaler 4
	//     - PPRE1[2:0] = 110 ->  prescaler 8
	//     - PPRE1[2:0] = 111 ->  prescaler 16
	//
	// 5) APB1 frequency = AHB frequency/ APB1 prescaler

	uint32_t pclk1,SystemClk;

	uint8_t clksrc,temp,ahbp,apb1p;

	clksrc = ((RCC->CFGR >> 2) & 0x3);

	if(clksrc == 0 )
	{
		SystemClk = 16000000;
	}else if(clksrc == 1)
	{
		SystemClk = 8000000;
	}else if (clksrc == 2)
	{
		SystemClk = RCC_GetPLLOutputClock();
	}

	//for ahb
	temp = ((RCC->CFGR >> 4 ) & 0xF);

	if(temp < 8)
	{
		ahbp = 1;
	}else
	{
		ahbp = AHB_PreScaler[temp-8];
	}



	//apb1
	temp = ((RCC->CFGR >> 10 ) & 0x7);

	if(temp < 4)
	{
		apb1p = 1;
	}else
	{
		apb1p = APB1_PreScaler[temp-4];
	}

	pclk1 =  (SystemClk / ahbp) /apb1p;

	return pclk1;
}



/*********************************************************************
 * @fn      		  - RCC_GetPCLK2Value
 *
 * @brief             - This function provides the frequency of APB2 clock
 *
 * @param[in]         - none
 *
 * @return            - pclk2 - APB2 frequency
 *
 * @Note              - Assumes HSI = 16 MHz and HSE = 8 MHz.
 *
 * @Abstraction       - This function abstracts clock frequency calculations and
 *                      register accessing from the application, allowing the application
 *                      to read the APB2 clock frequency through a simple API call.
 *
 */
uint32_t RCC_GetPCLK2Value(void)
{
	    // 1) Find the system clock by checking SWS bits in CFGR register
		//     - SWS[1:0] = 00 -> HSI
		//     - SWS[1:0] = 01 -> HSE
		//     - SWS[1:0] = 10 -> PLL
		//
		// 2) Find AHB prescaler by checking HPRE bits in CFGR
		//     - HPRE[3:0] = 0xxx ->  prescaler 1 (no division)
		//     - HPRE[3:0] = 1000 ->  prescaler 2
		//     - HPRE[3:0] = 1001 ->  prescaler 4
		//     - HPRE[3:0] = 1010 ->  prescaler 8
		//     - HPRE[3:0] = 1011 ->  prescaler 16
		//     - HPRE[3:0] = 1100 ->  prescaler 64
		//     - HPRE[3:0] = 1101 ->  prescaler 128
		//     - HPRE[3:0] = 1110 ->  prescaler 256
		//     - HPRE[3:0] = 1111 ->  prescaler 5121
		//
		// 3) AHB frequency = system clock frequency/ AHB prescaler
		//
		// 4) Find APB2 prescaler by checing PPRE2 bits in CFGR
		//     - PPRE2[2:0] = 0xx ->  prescaler 1 (no division)
		//     - PPRE2[2:0] = 100 ->  prescaler 2
		//     - PPRE2[2:0] = 101 ->  prescaler 4
		//     - PPRE2[2:0] = 110 ->  prescaler 8
		//     - PPRE2[2:0] = 111 ->  prescaler 16
		//
		// 5) APB2 frequency = AHB frequency/ APB2 prescaler

	uint32_t SystemClock=0,tmp,pclk2;
	uint8_t clk_src = ( RCC->CFGR >> 2) & 0X3;

	uint8_t ahbp,apb2p;

	if(clk_src == 0)
	{
		SystemClock = 16000000;
	}else
	{
		SystemClock = 8000000;
	}
	tmp = (RCC->CFGR >> 4 ) & 0xF;

	if(tmp < 0x08)
	{
		ahbp = 1;
	}else
	{
       ahbp = AHB_PreScaler[tmp-8];
	}

	tmp = (RCC->CFGR >> 13 ) & 0x7;
	if(tmp < 0x04)
	{
		apb2p = 1;
	}else
	{
		apb2p = APB1_PreScaler[tmp-4];
	}

	pclk2 = (SystemClock / ahbp )/ apb2p;

	return pclk2;
}
/*********************************************************************
 * @fn      		  - RCC_GetPLLOutputClock
 *
 * @brief             - This function calculates and returns the PLL output clock frequency.
 *
 * @param[in]         - none
 *
 * @return            - pll_output - PLL output clock frequency in Hz (returns zero if PLL configuration is invalid)
 *
 * @Note              - Assumes HSI = 16 MHz and HSE = 8 MHz.
 *
 * @Abstraction       - This function abstracts clock frequency calculations and
 *                      register accessing from the application, allowing the application
 *                      to read the PLL clock frequency through a simple API call.
 *
 */
uint32_t  RCC_GetPLLOutputClock()
{
	// PLL output frequency depends on:
	//
	// - System clock source for PLL (HSI/HSE)
	//
	// - PLLM division factor
	//                     * VCO i/p frequency = system clock frequency / PLLM
	//
	// - PLLN multiplication factor
	//                     * VCO o/p frequency = VCO i/p frequency x  PLLN
	//
	// - PLLP division factor
	//                     * PLL output clock = VCO o/p frequency / PLLP



	/*********************************** PLL CLOCK SOURCE ****************************************/

	// PLL clock sorce is set in PLLCFGR_SRC[22]
	uint32_t PLLSrcClock=0;
	uint8_t clk_src = ( RCC->PLLCFGR >> 22) & 0X1;

	if(clk_src == 0)
	{
		PLLSrcClock = 16000000;
	}else
	{
		PLLSrcClock = 8000000;
	}


	/*********************************** VCO INPUT CLOCK ****************************************/

	// PLLM factor is set in PLLCFGR_PLLM[5:0] bits
	// Valid PLLM range: 2<= PLLM <=62
	// Valid VCO i/p frequency range is between 1MHz and 2MHz

	uint8_t PLLM = (RCC->PLLCFGR & 0x3F);
	if(PLLM<2 || PLLM>62){return 0;}// wrong PLL configuration

	uint32_t vco_input = PLLSrcClock / PLLM;
	if(vco_input<1000000 || vco_input>2000000){return 0;}// wrong PLL configuration


	/*********************************** VCO OUTPUT CLOCK ****************************************/

	// PLLN factor is set in PLLCFGR_PLLN[14:6] bits
	// Valid PLLN range: 192<= PLLN <=432
	// Valid VCO o/p frequency range is between 192MHz and 432MHz

	uint16_t PLLN = ((RCC->PLLCFGR>> 6) & 0x1FF);
	if(PLLN<192 || PLLN>432){return 0;}// wrong PLL configuration

	uint32_t vco_output = vco_input * PLLN;
	if(vco_output<192000000 || vco_output>432000000){return 0;}// wrong PLL configuration


	/*********************************** PLL OUTPUT CLOCK ****************************************/

	// PLLP factor is set in PLLCFGR_PLLP[17:16] bits
	// Valid PLL o/p frequency range in F401 is below 84MHz

	uint16_t PLLP = ((RCC->PLLCFGR>> 16) & 0x3);

	// PLLP prescaler values = { 2, 4, 6, 8}
	uint8_t PLLP_PSC[] = { 2, 4, 6, 8};

	uint32_t pll_output = vco_output / PLLP_PSC[PLLP];
	if(pll_output> 84000000){return 0;}// wrong PLL configuration


	return pll_output;
}





































