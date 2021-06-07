/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#define VD2_GPIO_PORT			GPIOB
#define VD2_GPIO_PIN			GPIO_Pin_11
#define VD2_GPIO_RCC 			RCC_APB2Periph_GPIOB

void TIM3_IRQHandler(void) {
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		VD2_GPIO_PORT->ODR ^= VD2_GPIO_PIN;
	}
}

static void PINOutODInitilize() {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(VD2_GPIO_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = VD2_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(VD2_GPIO_PORT, &GPIO_InitStructure);
}

static void TIMER3Initialze() {
	TIM_TimeBaseInitTypeDef TIMER_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructInit(&TIMER_InitStructure);
	TIMER_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIMER_InitStructure.TIM_Prescaler = 12000;
	TIMER_InitStructure.TIM_Period = 1000; //1s
	TIM_TimeBaseInit(TIM3, &TIMER_InitStructure);

	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(TIM3_IRQn, NVIC_EncodePriority(prioritygroup, 2, 1));
	NVIC_EnableIRQ(TIM3_IRQn);

	TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}

void LedInit() {
	PINOutODInitilize();
	TIMER3Initialze();
}
