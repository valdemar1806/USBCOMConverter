/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_DESC_H_
#define USB_DESC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/_stdint.h>
#include "usb_def.h"

uint8_t * GetDeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * GetLangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * GetProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * GetManufacturerStrDescriptor(USBD_SpeedTypeDef speed,
		uint16_t *length);
uint8_t * GetSerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * GetConfigurationStrDescriptor(USBD_SpeedTypeDef speed,
		uint16_t *length);
uint8_t * GetInterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

uint8_t * GetFSConfigDescriptor(uint16_t *length);
uint8_t * GetDeviceQualifierDescriptor(uint16_t *length);

#ifdef __cplusplus
}
#endif

#endif /* USB_DESC_H_ */
