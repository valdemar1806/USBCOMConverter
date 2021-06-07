/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_CORE_H_
#define USB_CORE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "usb_device.h"

void USB_IRQHandler(_USBDevice* device);

#ifdef __cplusplus
}
#endif

#endif /* USB_CORE_H_ */
