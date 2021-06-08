/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "stm32f10x_tim.h"
#include "cdc_usart.h"
#include "usb_cdc.h"
#include "usb_io.h"
#include "usb_ll.h"
#include "core_cmFunc.h"
#include "string.h"

#define CNTR_RX_20 (20) //20*5ms=100ms
#define CNTR_TX_20 (20) //20*5ms=100ms

static uint8_t decRX20 = 0;
static uint8_t decTX20 = 0;

static uint8_t cntrRX20 = CNTR_RX_20;
static uint8_t cntrTX20 = CNTR_TX_20;

#define CDC_VULUME_SIZE (CDC_DATA_FS_MAX_PACKET_SIZE)

#define CDC_RX_BUFFER_SIZE (8*CDC_VULUME_SIZE)
#define CDC_RX_BUFFER_SIZE_HALF (CDC_RX_BUFFER_SIZE>>1)
#define CDC_TX_BUFFER_SIZE (32*CDC_VULUME_SIZE)
#define CDC_TX_BUFFER_SIZE_HALF (CDC_TX_BUFFER_SIZE>>1)

static uint8_t cdc_rx_buffer[CDC_RX_BUFFER_SIZE];
static uint8_t cdc_tx_buffer[CDC_TX_BUFFER_SIZE];

#ifndef _DEBUG_
#define _DEBUG_
#endif

#ifdef _DEBUG_
static uint32_t total_usb_rx = 0;
static uint32_t total_usb_tx = 0;
static uint32_t total_cdc_tx = 0;
#endif

static uint16_t idxUSBRX = 0;
static uint16_t nUSBRX = 0;
static uint16_t idxCDCTX = 0;
static uint16_t nCDCTX = 0;
static uint16_t idxUSBTX = 0;

static uint8_t RxBuf[2 * CDC_DATA_FS_MAX_PACKET_SIZE];

static uint8_t CmdBuf[2 * 8];

void TIM4_IRQHandler(void) {
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		decRX20 = 1;
		decTX20 = 1;
	}
}

static void TIMER4Initialize() {
	TIM_TimeBaseInitTypeDef TIMER_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseStructInit(&TIMER_InitStructure);
	TIMER_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIMER_InitStructure.TIM_Prescaler = 12000;
	TIMER_InitStructure.TIM_Period = 5; //5ms
	TIM_TimeBaseInit(TIM4, &TIMER_InitStructure);

	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(TIM4_IRQn, NVIC_EncodePriority(prioritygroup, 2, 2));
	NVIC_EnableIRQ(TIM4_IRQn);

	TIM_Cmd(TIM4, ENABLE);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
}

static uint16_t CDC_GetIdxDiff(void) {
	__disable_irq();
	uint16_t diff = (nUSBRX - nCDCTX) * CDC_TX_BUFFER_SIZE + idxUSBRX
			- idxCDCTX;
	__enable_irq();
	return diff;
}

static uint8_t CDC_IsIdxDiffMoreThan(uint16_t diff) {
	return CDC_GetIdxDiff() >= diff;
}

static void CDC_FlushRXData(_USBDevice* device) {
	if (decRX20) {
		decRX20 = 0;
		cntrRX20--;
	}
	if (cntrRX20) {
		return;
	}
	cntrRX20 = CNTR_RX_20;

	__disable_irq();

	uint16_t curIdx = CDC_RX_BUFFER_SIZE - CDCUSARTGetRXDataCounter();

	if (idxUSBTX == curIdx) {

		__enable_irq();

		return;
	}
	uint16_t idx = idxUSBTX;
	uint16_t len = curIdx - idxUSBTX;

	EPSendData(CDC_IN_EP & EP_ADDR_MSK, device, &cdc_rx_buffer[idx], len);
#ifdef _DEBUG_
	total_usb_tx += len;
#endif
	idxUSBTX = curIdx;

	__enable_irq();
}

static void CDC_SendData(uint16_t len) {
	CDCUSARTInitSendData(&cdc_tx_buffer[idxCDCTX], len);
#ifdef _DEBUG_
	total_cdc_tx += len;
#endif
	if (idxCDCTX + len >= CDC_TX_BUFFER_SIZE) {
		idxCDCTX = (idxCDCTX + len) % CDC_TX_BUFFER_SIZE;
		nCDCTX++;
	} else {
		idxCDCTX += len;
	}
	CDCUSARTWaitSendData();
}

static void CDC_FlushTXData(_USBDevice* device) {
	if (decTX20) {
		decTX20 = 0;
		cntrTX20--;
	}
	if (cntrTX20) {
		return;
	}
	cntrTX20 = CNTR_TX_20;

	uint16_t available = CDC_GetIdxDiff();

	if (!available) {
		return;
	}
	do {
		if (available > CDC_VULUME_SIZE) {
			available = CDC_VULUME_SIZE;
		}
		CDC_SendData(available);
		available = CDC_GetIdxDiff();
	} while (available);

	__disable_irq();
	idxUSBRX = nUSBRX = 0;
	__enable_irq();

	idxCDCTX = nCDCTX = 0;

	EPRxValid(CDC_OUT_EP & EP_ADDR_MSK, device);
}

uint8_t CDC_Init(_USBDevice *device, uint8_t cfgidx) {
	_EPoint* ep = &device->OUT_EP[CDC_OUT_EP & EP_ADDR_MSK];
	EPActivate(ep);
	ep = &device->IN_EP[CDC_IN_EP & EP_ADDR_MSK];
	EPActivate(ep);
	ep = &device->IN_EP[CDC_CMD_EP & EP_ADDR_MSK];
	EPActivate(ep);

	ep = &device->OUT_EP[CDC_OUT_EP & EP_ADDR_MSK];
	EPPrepareRx(CDC_OUT_EP & EP_ADDR_MSK, device, RxBuf, ep->MaxSize);

	ep = &device->IN_EP[CDC_IN_EP & EP_ADDR_MSK];
	ep->pData = NULL;
	ep->DataLen = 0;
	return 0;
}

uint8_t CDC_DeInit(_USBDevice *device, uint8_t cfgidx) {
	_EPoint* ep = &device->IN_EP[CDC_OUT_EP & EP_ADDR_MSK];
	EPDeactivate(ep);
	ep = &device->IN_EP[CDC_IN_EP & EP_ADDR_MSK];
	EPDeactivate(ep);
	ep = &device->IN_EP[CDC_CMD_EP & EP_ADDR_MSK];
	EPDeactivate(ep);
	return 0;
}

uint8_t CDC_Setup(_USBDevice* device, USBD_SetupReqTypedef* request) {
	uint8_t ifalt = 0U;
	uint16_t status_info = 0U;

	switch (request->bmRequest & USB_REQ_TYPE_MASK) {
	case USB_REQ_TYPE_CLASS:
		if (request->wLength) {
			if (request->bmRequest & 0x80U) {

				GetCDCUSARTConfig(CmdBuf);

				EPSendData(0, device, CmdBuf, request->wLength);
			} else {
				device->CmdOpCode = request->bRequest;
				device->CmdLength = (uint8_t) request->wLength;

				EPPrepareRx(0, device, CmdBuf, request->wLength);
			}
			return 0;
		}
		EPSendStatus(0, device);
		return 0;

	case USB_REQ_TYPE_STANDARD:
		switch (request->bRequest) {
		case USB_REQ_GET_STATUS:
			if (device->dev_state == USBD_STATE_CONFIGURED) {
				EPSendData(0, device, (uint8_t *) &status_info, 2U);
				return 0;
			}
			break;

		case USB_REQ_GET_INTERFACE:
			if (device->dev_state == USBD_STATE_CONFIGURED) {
				EPSendData(0, device, &ifalt, 1U);
				return 0;
			}
			break;

		case USB_REQ_SET_INTERFACE:
			if (device->dev_state == USBD_STATE_CONFIGURED) {
				EPSendData(0, device, &ifalt, 1U);
				return 0;
			}
			break;
		}
		break;
	}
	EPError(0);
	return 1;
}

uint8_t CDC_EP0_RxReady(_USBDevice* device) {
	if (device->CmdOpCode != 0xFFU && device->CmdLength != 0) {

		SetCDCUSARTConfig(CmdBuf);

		CDCUSARTInit(cdc_rx_buffer, CDC_RX_BUFFER_SIZE);
		TIMER4Initialize();

		idxCDCTX = idxUSBRX = nCDCTX = nUSBRX = idxUSBTX = 0;

		device->CmdOpCode = 0xFFU;
		device->CmdLength = 0;
	}
	return 0;
}

uint8_t CDC_RxReady(_USBDevice* device) {
	_EPoint* epOut = &device->OUT_EP[CDC_OUT_EP & EP_ADDR_MSK];
#ifdef _DEBUG_
	total_usb_rx += epOut->DataLen;
#endif
	if ((idxUSBRX + epOut->DataLen) < CDC_TX_BUFFER_SIZE) {
		memcpy(&cdc_tx_buffer[idxUSBRX], epOut->pData, epOut->DataLen);
		idxUSBRX += epOut->DataLen;
	} else {
		uint16_t tail = idxUSBRX + epOut->DataLen - CDC_TX_BUFFER_SIZE;
		memcpy(&cdc_tx_buffer[idxUSBRX], epOut->pData, epOut->DataLen - tail);
		idxUSBRX = 0;
		if (tail != 0) {
			memcpy(&cdc_tx_buffer[idxUSBRX],
					epOut->pData + epOut->DataLen - tail, tail);
			idxUSBRX += tail;
		}
		nUSBRX++;
	}
	if (!CDC_IsIdxDiffMoreThan(16 * CDC_VULUME_SIZE)) {
		EPRxValid(CDC_OUT_EP & EP_ADDR_MSK, device);
	}
	cntrTX20 = CNTR_TX_20;
	return 0;
}

void CDCUSART_RXHALF_Callback(_USBDevice* device) {
	cntrRX20 = CNTR_RX_20;
	uint16_t len = CDC_RX_BUFFER_SIZE_HALF - idxUSBTX;
	EPSendData(CDC_IN_EP & EP_ADDR_MSK, device, &cdc_rx_buffer[idxUSBTX], len);
	idxUSBTX = CDC_RX_BUFFER_SIZE_HALF;
#ifdef _DEBUG_
	total_usb_tx += len;
#endif
}

void CDCUSART_RXFULL_Callback(_USBDevice* device) {
	cntrRX20 = CNTR_RX_20;
	uint16_t len = CDC_RX_BUFFER_SIZE - idxUSBTX;
	EPSendData(CDC_IN_EP & EP_ADDR_MSK, device, &cdc_rx_buffer[idxUSBTX], len);
	idxUSBTX = 0;
#ifdef _DEBUG_
	total_usb_tx += len;
#endif
}

void CDC_Loop(_USBDevice* device) {
	if (CDC_IsIdxDiffMoreThan(8 * CDC_VULUME_SIZE)) {
		CDC_SendData(8 * CDC_VULUME_SIZE);
		if (!CDC_IsIdxDiffMoreThan(16 * CDC_VULUME_SIZE)) {
			EPRxValid(CDC_OUT_EP & EP_ADDR_MSK, device);
		}
	}
	CDC_FlushRXData(device);
	CDC_FlushTXData(device);
}
