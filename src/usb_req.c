/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include <stddef.h>
#include "usb_cdc.h"
#include "usb_desc.h"
#include "usb_io.h"
#include "usb_ll.h"

static void GetDescriptor(_USBDevice* device, USBD_SetupReqTypedef* request) {
	uint16_t len = 0;
	uint8_t *pbuf = NULL;

	switch (request->wValue >> 8) {
	case USB_DESC_TYPE_DEVICE:
		pbuf = GetDeviceDescriptor(USBD_SPEED_FULL, &len);
		break;

	case USB_DESC_TYPE_CONFIGURATION:
		pbuf = GetFSConfigDescriptor(&len);
		break;

	case USB_DESC_TYPE_STRING:
		switch ((uint8_t) (request->wValue)) {
		case USBD_IDX_LANGID_STR:
			pbuf = GetLangIDStrDescriptor(USBD_SPEED_FULL, &len);
			break;

		case USBD_IDX_MFC_STR:
			pbuf = GetManufacturerStrDescriptor(USBD_SPEED_FULL, &len);
			break;

		case USBD_IDX_PRODUCT_STR:
			pbuf = GetProductStrDescriptor(USBD_SPEED_FULL, &len);
			break;

		case USBD_IDX_SERIAL_STR:
			pbuf = GetSerialStrDescriptor(USBD_SPEED_FULL, &len);
			break;

		case USBD_IDX_CONFIG_STR:
			pbuf = GetConfigurationStrDescriptor(USBD_SPEED_FULL, &len);
			break;

		case USBD_IDX_INTERFACE_STR:
			pbuf = GetInterfaceStrDescriptor(USBD_SPEED_FULL, &len);
			break;
		}
		break;

	case USB_DESC_TYPE_DEVICE_QUALIFIER:
		pbuf = GetDeviceQualifierDescriptor(&len);
		break;

	case USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
		break;
	}
	if (pbuf == NULL) {
		EPError(0);
		return;
	}
	if ((len != 0) && (request->wLength != 0)) {
		len = MIN(len, request->wLength);
		EPSendData(0, device, pbuf, len);
	}
	if (request->wLength == 0) {
		EPSendStatus(0, device);
	}
}

static void SetAddress(_USBDevice* device, USBD_SetupReqTypedef* request) {
	if (device->dev_state == USBD_STATE_ADDRESSED) {
		EPError(0);
		return;
	}
	if (!((request->wIndex == 0) && (request->wLength == 0)
			&& (request->wValue < 128))) {
		EPError(0);
		return;
	}
	device->dev_state = USBD_STATE_ADDRESSED;
	uint8_t address = (uint8_t) (request->wValue) & 0x7F;
	device->USB_Address = address;
	EPSendStatus(0, device);
}

static void SetConfig(_USBDevice* device, USBD_SetupReqTypedef* request) {
	static uint8_t cfgidx;

	cfgidx = (uint8_t) (request->wValue);

	if (cfgidx > USBD_MAX_NUM_CONFIGURATION) {
		EPError(0);
		return;
	}
	switch (device->dev_state) {
	case USBD_STATE_ADDRESSED:
		if (cfgidx == 1U) {
			if (CDC_Init(device, 1U)) {
				EPError(0);
				return;
			}
			device->dev_state = USBD_STATE_CONFIGURED;
		}
		break;

	case USBD_STATE_CONFIGURED:
		if (cfgidx == 0U) {
			CDC_DeInit(device, 0U);
			device->dev_state = USBD_STATE_ADDRESSED;
		}
		break;

	default:
		CDC_DeInit(device, cfgidx);
		EPError(0);
		return;
	}
	EPSendStatus(0, device);
}

static void GetConfig(_USBDevice* device, USBD_SetupReqTypedef* request) {
	uint8_t dev_config;

	if (request->wLength != 1U) {
		EPError(0);
		return;
	}
	switch (device->dev_state) {
	case USBD_STATE_DEFAULT:
	case USBD_STATE_ADDRESSED:
		dev_config = 0;
		EPSendData(0, device, &dev_config, 1U);
		return;

	case USBD_STATE_CONFIGURED:
		dev_config = 1;
		EPSendData(0, device, &dev_config, 1U);
		return;
	}
	EPError(0);
}

static void StdDevReq(_USBDevice* device, USBD_SetupReqTypedef* request) {
	switch (request->bmRequest & USB_REQ_TYPE_MASK) {
	case USB_REQ_TYPE_CLASS:
	case USB_REQ_TYPE_VENDOR:
		break;

	case USB_REQ_TYPE_STANDARD:
		switch (request->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			GetDescriptor(device, request);
			break;

		case USB_REQ_SET_ADDRESS:
			SetAddress(device, request);
			break;

		case USB_REQ_SET_CONFIGURATION:
			SetConfig(device, request);
			break;

		case USB_REQ_GET_CONFIGURATION:
			GetConfig(device, request);
			break;

		case USB_REQ_GET_STATUS:
		case USB_REQ_SET_FEATURE:
		case USB_REQ_CLEAR_FEATURE:
			break;
		}
		break;
	}
}

static void StdItfReq(_USBDevice* device, USBD_SetupReqTypedef* request) {
	switch (request->bmRequest & USB_REQ_TYPE_MASK) {
	case USB_REQ_TYPE_CLASS:
	case USB_REQ_TYPE_VENDOR:
	case USB_REQ_TYPE_STANDARD:
		switch (device->dev_state) {
		case USBD_STATE_DEFAULT:
		case USBD_STATE_ADDRESSED:
		case USBD_STATE_CONFIGURED:
			if (LOBYTE(request->wIndex) <= USBD_MAX_NUM_INTERFACES) {
				CDC_Setup(device, request);
				return;
			}
			break;
		}
		break;
	}
	EPError(0);
}

static void ParseSetupRequest(USBD_SetupReqTypedef *request, uint8_t *bufRX) {
	request->bmRequest = *(uint8_t *) (bufRX);
	request->bRequest = *(uint8_t *) (bufRX + 1);
	request->wValue = SWAPBYTE(bufRX + 2);
	request->wIndex = SWAPBYTE(bufRX + 4);
	request->wLength = SWAPBYTE(bufRX + 6);
}

void SetupStage(_USBDevice* device) {
	USBD_SetupReqTypedef request;
	uint8_t bufRX[8];

	EPRXBitClear(0);

	uint16_t CntrRX = EPGetSingleCntrRX(0);

	ReadPMA(EPGetSingleAddrRX(0), bufRX, CntrRX);

	ParseSetupRequest(&request, bufRX);

	switch (request.bmRequest & 0x1F) {
	case USB_REQ_RECIPIENT_DEVICE:
		StdDevReq(device, &request);
		break;

	case USB_REQ_RECIPIENT_INTERFACE:
		StdItfReq(device, &request);
		break;

	case USB_REQ_RECIPIENT_ENDPOINT:
		break;
	}
}

void OutStage(uint8_t i, _USBDevice* device) {
	EPRXBitClear(i);

	_EPoint* ep = &device->OUT_EP[i];

	if (i) { //!0
		uint16_t CntrRX = EPGetSingleCntrRX(i);
		if ((ep->pData != NULL) && (CntrRX != 0)) {
			ReadPMA(EPGetSingleAddrRX(i), ep->pData, CntrRX);
			ep->DataLen = CntrRX;
			CDC_RxReady(device);
		}
	} else { //0
		uint16_t CntrRX = EPGetSingleCntrRX(0);
		if ((ep->pData != NULL) && (CntrRX != 0)) {
			ReadPMA(EPGetSingleAddrRX(0), ep->pData, CntrRX);
			ep->DataLen -= CntrRX;
			if (ep->DataLen) {
				ep->pData += CntrRX;
				EPContinueRx(0, device, ep->pData, ep->DataLen);
			} else {
				CDC_EP0_RxReady(device);
				EPSendStatus(0, device);
				EPRxValid(0, device);
			}
		}
	}
}

void InStage(uint8_t i, _USBDevice* device) {
	EPTXBitClear(i);

	_EPoint* ep = &device->IN_EP[i];

	if (i) { //!0
		if (ep->DataLen) {
			uint16_t CntrTX = EPGetSingleCntrTX(i);
			ep->pData += CntrTX;
			if (CntrTX) {
				EPContinueSend(i, device, ep->pData, ep->DataLen);
			}
		} else {
			EPRxValid(i, device);
		}
	} else { //0
		if (ep->DataLen) {
			uint16_t CntrTX = EPGetSingleCntrTX(0);
			ep->pData += CntrTX;
			EPContinueSend(0, device, ep->pData, ep->DataLen);
		} else {
			EPRxValid(0, device);
		}
		if (device->USB_Address) {
			USBSetDADDR(device->USB_Address);
			device->USB_Address = 0;
		}
	}
}
