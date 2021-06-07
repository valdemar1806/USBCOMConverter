/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_CDC_H_
#define USB_CDC_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/_stdint.h>
#include "usb_def.h"
#include "usb_device.h"

#define CDC_IN_EP                                   0x81U  /* EP1 for data IN */
#define CDC_OUT_EP                                  0x01U  /* EP1 for data OUT */
#define CDC_CMD_EP                                  0x82U  /* EP2 for CDC commands */

/* CDC Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */
#define CDC_DATA_HS_MAX_PACKET_SIZE                 512U  /* Endpoint IN & OUT Packet size */
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U  /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE                         8U  /* Control Endpoint Packet size */

#define CDC_DATA_HS_IN_PACKET_SIZE                  CDC_DATA_HS_MAX_PACKET_SIZE
#define CDC_DATA_HS_OUT_PACKET_SIZE                 CDC_DATA_HS_MAX_PACKET_SIZE

#define CDC_DATA_FS_IN_PACKET_SIZE                  CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 CDC_DATA_FS_MAX_PACKET_SIZE

#define CDC_FS_BINTERVAL                          0x10U
#define CDC_HS_BINTERVAL                          0x10U

#define  SWAPBYTE(addr)        (((uint16_t)(*((uint8_t *)(addr)))) + \
                               (((uint16_t)(*(((uint8_t *)(addr)) + 1U))) << 8U))

#define LOBYTE(x)  ((uint8_t)((x) & 0x00FFU))
#define HIBYTE(x)  ((uint8_t)(((x) & 0xFF00U) >> 8U))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

uint8_t CDC_Init(_USBDevice *device, uint8_t cfgidx);
uint8_t CDC_DeInit(_USBDevice *device, uint8_t cfgidx);
uint8_t CDC_Setup(_USBDevice* device, USBD_SetupReqTypedef* request);
uint8_t CDC_EP0_RxReady(_USBDevice* device);
uint8_t CDC_RxReady(_USBDevice* device);

void CDC_Loop(_USBDevice* device);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_H_ */
