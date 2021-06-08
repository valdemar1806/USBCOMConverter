/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef USB_LL_C_
#define USB_LL_C_

#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/_stdint.h>
#include "usb_device.h"

void USBReset();
void USBEnableInt();
void USBDisableInt();
uint16_t USBGetInterrupts();
void USBClearPendingInterruptBit(uint16_t interrupt);
void USBSetDADDR(uint8_t address);

void ReadPMA(uint16_t PMAAddress, uint8_t *bufRX, uint16_t CntrRX);
void WritePMA(uint16_t PMAAddress, uint8_t *bufTX, uint16_t CntrTX);

void EPSetReg(uint8_t i, uint16_t v);
uint16_t EPGetReg(uint8_t i);

void EPActivate(_EPoint *ep);
void EPDeactivate(_EPoint *ep);

void EPSetSingleAddrTX(uint8_t i, uint16_t addr);
void EPSetSingleCntrTX(uint8_t i, uint16_t cntr);
void EPSetSingleAddrRX(uint8_t i, uint16_t addr);
void EPSetSingleCntrRX(uint8_t i, uint16_t cntr);

uint16_t EPGetSingleAddrTX(uint8_t i);
uint16_t EPGetSingleCntrTX(uint8_t i);
uint16_t EPGetSingleAddrRX(uint8_t i);
uint16_t EPGetSingleCntrRX(uint8_t i);

void EPTXState(uint8_t i, uint16_t state);
void EPRXState(uint8_t i, uint16_t state);

void EPTXBitClear(uint8_t i);
void EPRXBitClear(uint8_t i);

void EPError(uint8_t i);

void WKUP_ISR_Handler();
void SUSP_ISR_Handler();

#ifdef __cplusplus
}
#endif

#endif /* USB_LL_C_ */
