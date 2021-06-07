/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/_stdint.h>

typedef struct {
	uint8_t EPAddr;
	uint8_t isIN;
	uint8_t EPType;
	uint16_t PMAAddress;
	uint16_t MaxSize;
	uint8_t* pData;
	uint16_t DataLen;
} _EPoint;

typedef struct USBDeviceStruct {
	uint8_t USB_Address;
	uint8_t dev_state;
	uint8_t old_dev_state;
	_EPoint IN_EP[8];
	_EPoint OUT_EP[8];
	uint8_t CmdOpCode;
	uint8_t CmdLength;
} _USBDevice;

void USBDeviceInit(_USBDevice* device);

#ifdef __cplusplus
}
#endif

#endif /* USB_DEVICE_H_ */
