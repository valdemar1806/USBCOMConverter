/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_REQ_H_
#define USB_REQ_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/_stdint.h>
#include "usb_device.h"

void SetupStage(_USBDevice* device);
void OutStage(uint8_t i, _USBDevice* device);
void InStage(uint8_t i, _USBDevice* device);

#ifdef __cplusplus
}
#endif

#endif /* USB_REQ_H_ */
