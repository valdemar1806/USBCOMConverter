/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "usb_device.h"

#define CDC_USART			USART2
#define CDC_USART_RCC 			RCC_APB1Periph_USART2
#define CDC_GPIO_RCC			RCC_APB2Periph_GPIOA

#define CDC_TX_PORT			GPIOA
#define CDC_TX_PIN			GPIO_Pin_2

#define CDC_RX_PORT			GPIOA
#define CDC_RX_PIN			GPIO_Pin_3

typedef struct {
	uint32_t baudRate;
	uint8_t stopBits;
	uint8_t parity;
	uint8_t dataLen;
} _USARTConfig;

static _USARTConfig USARTConfig = { (uint32_t) 115200U, 0U, 0U, 0x08U };

extern _USBDevice USBDevice;

extern void CDCUSART_RXHALF_Callback(_USBDevice* device);
extern void CDCUSART_RXFULL_Callback(_USBDevice* device);

void DMA1_Channel6_IRQHandler(void) {
	if (DMA_GetITStatus(DMA1_IT_HT6) == SET) {
		DMA_ClearITPendingBit(DMA1_IT_HT6);
		CDCUSART_RXHALF_Callback(&USBDevice);
	}
	if (DMA_GetITStatus(DMA1_IT_TC6) == SET) {
		DMA_ClearITPendingBit(DMA1_IT_TC6);
		CDCUSART_RXFULL_Callback(&USBDevice);
	}
}

static void PINAFOutInitialize(GPIO_TypeDef* port, uint32_t pin, uint32_t rcc) {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB1PeriphClockCmd(rcc, ENABLE);

	GPIO_InitStructure.GPIO_Pin = pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(port, &GPIO_InitStructure);
}

static void PINAFInInitialize(GPIO_TypeDef* port, uint32_t pin, uint32_t rcc) {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB1PeriphClockCmd(rcc, ENABLE);

	GPIO_InitStructure.GPIO_Pin = pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(port, &GPIO_InitStructure);
}

static void CDCUSARTDMARXInitialize(uint8_t* memoryBaseAddr,
		uint16_t bufferSize) {
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_Cmd(DMA1_Channel6, DISABLE);

	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(CDC_USART->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) memoryBaseAddr;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = bufferSize;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel6, &DMA_InitStructure);

	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(DMA1_Channel6_IRQn,
			NVIC_EncodePriority(prioritygroup, 3, 1));
	NVIC_EnableIRQ(DMA1_Channel6_IRQn);

	DMA_ITConfig(DMA1_Channel6, DMA_IT_HT, ENABLE);
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

	DMA_Cmd(DMA1_Channel6, ENABLE);
}

static void CDCUSARTDMATXInitialize(uint8_t* memoryBaseAddr,
		uint16_t bufferSize) {
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_Cmd(DMA1_Channel7, DISABLE);

	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(CDC_USART->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) memoryBaseAddr;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = bufferSize;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel7, &DMA_InitStructure);

	DMA_Cmd(DMA1_Channel7, ENABLE);
}

static void CDCUSARTInitialize(uint8_t* memoryBaseAddr, uint16_t bufferSize) {
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(CDC_GPIO_RCC, ENABLE);

	/* Configure CDC_USART Tx  as alternate function push-pull */
	PINAFOutInitialize(CDC_TX_PORT, CDC_TX_PIN, CDC_USART_RCC); //APB1!

	/* Configure CDC_USART Rx as input floating */
	PINAFInInitialize(CDC_RX_PORT, CDC_RX_PIN, CDC_USART_RCC); //APB1!

	USART_DMACmd(CDC_USART, USART_DMAReq_Rx, DISABLE);
	USART_DMACmd(CDC_USART, USART_DMAReq_Tx, DISABLE);

	USART_Cmd(CDC_USART, DISABLE);

	/* PC_USART configuration ------------------------------------------------------*/
	/* PC_USART configured as follow:
	 - BaudRate = 115200 baud
	 - Word Length = 8 Bits
	 - One Stop Bit
	 - No parity
	 - Hardware flow control disabled (RTS and CTS signals)
	 - Receive and transmit enabled
	 - USART Clock disabled
	 - USART CPOL: Clock is active low
	 - USART CPHA: Data is captured on the middle
	 - USART LastBit: The clock pulse of the last data bit is not output to
	 the SCLK pin
	 */

	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate = (uint32_t) USARTConfig.baudRate;
	switch (USARTConfig.dataLen) {
	default:
	case 8:
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		break;
	case 9:
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		break;
	}
	switch (USARTConfig.stopBits) {
	default:
	case 0:
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		break;
	case 1:
		USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
		break;
	case 2:
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
		break;
	}
	switch (USARTConfig.parity) {
	default:
	case 0:
		USART_InitStructure.USART_Parity = USART_Parity_No;
		break;
	case 1:
		USART_InitStructure.USART_Parity = USART_Parity_Odd;
		break;
	case 2:
		USART_InitStructure.USART_Parity = USART_Parity_Even;
		break;
	}
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(CDC_USART, &USART_InitStructure);

	CDCUSARTDMARXInitialize(memoryBaseAddr, bufferSize);

	USART_Cmd(CDC_USART, ENABLE);

	USART_DMACmd(CDC_USART, USART_DMAReq_Rx, ENABLE);
	USART_DMACmd(CDC_USART, USART_DMAReq_Tx, ENABLE);
}

static uint16_t CDCUSARTDMARXGetCurrDataCounter() {
	return DMA_GetCurrDataCounter(DMA1_Channel6);
}

void GetCDCUSARTConfig(uint8_t* pBuf) {
	*(uint32_t*) pBuf = USARTConfig.baudRate;
	pBuf[4] = USARTConfig.stopBits;
	pBuf[5] = USARTConfig.parity;
	pBuf[6] = USARTConfig.dataLen;
}

void SetCDCUSARTConfig(uint8_t* pBuf) {
	USARTConfig.baudRate = *(uint32_t*) pBuf;
	USARTConfig.stopBits = pBuf[4];
	USARTConfig.parity = pBuf[5];
	USARTConfig.dataLen = pBuf[6];
}

void CDCUSARTInit(uint8_t* memoryBaseAddr, uint16_t bufferSize) {
	CDCUSARTInitialize(memoryBaseAddr, bufferSize);
}

void CDCUSARTInitSendData(uint8_t* memoryBaseAddr, uint16_t bufferSize) {
	CDCUSARTDMATXInitialize(memoryBaseAddr, bufferSize);
}

void CDCUSARTWaitSendData() {
	while (DMA_GetFlagStatus(DMA1_FLAG_TC7) == RESET) {
	}
	DMA_ClearFlag(DMA1_FLAG_TC7);
}

uint16_t CDCUSARTGetRXDataCounter() {
	return CDCUSARTDMARXGetCurrDataCounter();
}
