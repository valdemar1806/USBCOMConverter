/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include <stddef.h>
#include "usb_ll.h"
#include "ext_def.h"

void EPContinueSend(uint8_t i, _USBDevice* device, uint8_t *bufTX,
		uint16_t CntrTX) {
	_EPoint* ep = &device->IN_EP[i];
	if (CntrTX > ep->MaxSize) {
		CntrTX = ep->MaxSize;
	}
	ep->DataLen -= CntrTX;

	WritePMA(EPGetSingleAddrTX(i), bufTX, CntrTX);

	EPSetSingleCntrTX(i, CntrTX);

	EPTXState(i, USB_EP_TX_VALID);
}

void EPSendData(uint8_t i, _USBDevice* device, uint8_t *bufTX, uint16_t CntrTX) {
	_EPoint* ep = &device->IN_EP[i];
	ep->pData = bufTX;
	ep->DataLen = CntrTX;
	EPContinueSend(i, device, bufTX, CntrTX);
}

void EPSendStatus(uint8_t i, _USBDevice* device) {
	_EPoint* ep = &device->IN_EP[i];
	ep->pData = NULL;
	ep->DataLen = 0;
	EPContinueSend(i, device, NULL, 0);
}

void EPContinueRx(uint8_t i, _USBDevice* device, uint8_t *bufRX,
		uint16_t CntrRX) {
	_EPoint* ep = &device->OUT_EP[i];
	if (CntrRX > ep->MaxSize) {
		CntrRX = ep->MaxSize;
	}
	EPSetSingleCntrRX(i, CntrRX);

	EPRXState(i, USB_EP_RX_VALID);
}

void EPPrepareRx(uint8_t i, _USBDevice* device, uint8_t *bufRX, uint16_t CntrRX) {
	_EPoint* ep = &device->OUT_EP[i];
	ep->pData = bufRX;
	ep->DataLen = CntrRX;
	EPContinueRx(i, device, bufRX, CntrRX);
}

void EPRxValid(uint8_t i, _USBDevice* device) {
	_EPoint* ep = &device->OUT_EP[i];
	EPSetSingleCntrRX(i, ep->MaxSize);
	EPRXState(i, USB_EP_RX_VALID);
}

void EPRxStall(uint8_t i, _USBDevice* device) {
	EPSetSingleCntrRX(i, 0);
	EPRXState(i, USB_EP_RX_STALL);
}
