/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "stm32f10x_rcc.h"

void SetSysClock() {
	ErrorStatus HSEStartUpStatus;
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration -----------------------------*/
	/* RCC system reset(for debug purpose) */
	RCC_DeInit();

	/* Enable HSE */
	RCC_HSEConfig(RCC_HSE_ON);

	/* Wait till HSE is ready */
	HSEStartUpStatus = RCC_WaitForHSEStartUp();

	if (HSEStartUpStatus == SUCCESS) {
		/* PLLCLK = HSE*6 */
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6); //48MHz

		/* HCLK = SYSCLK/4 */
		RCC_HCLKConfig(RCC_SYSCLK_Div4); //12MHz

		/* PCLK1 = HCLK/1 */
		RCC_PCLK1Config(RCC_HCLK_Div1); //12MHz

		/* PCLK2 = HCLK/1 */
		RCC_PCLK2Config(RCC_HCLK_Div1); //12MHz

		/* USB Prescaler = 1 */
		RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1); //48MHz

		/* Enable PLL */
		RCC_PLLCmd(ENABLE);

		/* Wait till PLL is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {
		}

		/* Select PLL as system clock source */
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		/* Wait till PLL is used as system clock source */
		while (RCC_GetSYSCLKSource() != 0x08) {
		}
	} else { /* If HSE fails to start-up, the application will have wrong clock configuration.
	 User can add here some code to deal with this error */

		/* Go to infinite loop */
		while (1) {
		}
	}
}
