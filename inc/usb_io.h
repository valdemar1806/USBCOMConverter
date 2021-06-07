/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_IO_H_
#define USB_IO_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/_stdint.h>
#include "usb_device.h"

void EPContinueSend(uint8_t i, _USBDevice* device, uint8_t *bufTX, uint16_t CntrTX);
void EPSendData(uint8_t i, _USBDevice* device, uint8_t *bufTX, uint16_t CntrTX);
void EPSendStatus(uint8_t i, _USBDevice* device);

void EPContinueRx(uint8_t i, _USBDevice* device, uint8_t *bufRX, uint16_t CntrRX);
void EPPrepareRx(uint8_t i, _USBDevice* device, uint8_t *bufRX, uint16_t CntrRX);

void EPRxValid(uint8_t i, _USBDevice* device);
void EPRxStall(uint8_t i, _USBDevice* device);

#ifdef __cplusplus
}
#endif

#endif /* USB_IO_H_ */
