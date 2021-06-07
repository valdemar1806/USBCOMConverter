/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include <stddef.h>
#include "stm32f10x_rcc.h"
#include "usb_cdc.h"
#include "usb_core.h"
#include "usb_ll.h"

extern _USBDevice USBDevice;

void USB_LP_CAN1_RX0_IRQHandler(void) {
	USB_IRQHandler(&USBDevice);
}

static void USBInitialize() {
	RCC_APB1PeriphClockCmd(RCC_APB1ENR_USBEN, ENABLE);

	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn,
			NVIC_EncodePriority(prioritygroup, 0, 0));
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

static void EPConfig(_USBDevice* device, uint8_t EPAddr, uint16_t PMAAddress,
		uint8_t EPType, uint16_t MaxSize) {
	_EPoint* ep;
	if (EPAddr & 0x80) {
		ep = &device->IN_EP[EPAddr & EP_ADDR_MSK];
		ep->isIN = 1;
	} else {
		ep = &device->OUT_EP[EPAddr];
		ep->isIN = 0;
	}
	ep->EPAddr = EPAddr;
	ep->PMAAddress = PMAAddress;
	ep->EPType = EPType;
	ep->MaxSize = MaxSize;
}

void USBDeviceInit(_USBDevice* device) {
	device->USB_Address = 0;
	device->dev_state = USBD_STATE_DEFAULT;
	device->CmdOpCode = 0xFF;
	device->CmdLength = 0;

	USBDisableInt();

	EPConfig(device, 0x00, 0x40, USBD_EP_TYPE_CTRL, USB_MAX_EP0_SIZE);
	EPConfig(device, 0x80, 0x80, USBD_EP_TYPE_CTRL, USB_MAX_EP0_SIZE);
	/* Attention:
	 * I will set the COUNT1_RX value (in EPSetSingleCntrRX() function) with a margin
	 * for the receiving end point. If you set the value to CDC_DATA_FS_MAX_PACKET_SIZE,
	 * then when receiving packets with a length of CDC_DATA_FS_MAX_PACKET_SIZE,
	 * there WILL necessarily BE (rarely) collisions.*/
	EPConfig(device, CDC_OUT_EP, 0x100, USBD_EP_TYPE_BULK,
			2 * CDC_DATA_FS_MAX_PACKET_SIZE);
	EPConfig(device, CDC_IN_EP, 0x180, USBD_EP_TYPE_BULK,
	CDC_DATA_FS_MAX_PACKET_SIZE);
	EPConfig(device, CDC_CMD_EP, 0x200, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);

	_EPoint* ep = &device->OUT_EP[0];
	ep->pData = NULL;
	ep->DataLen = 0;

	USBInitialize();

	USBReset();

	USBEnableInt();
}
