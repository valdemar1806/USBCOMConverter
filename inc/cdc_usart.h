/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#ifndef CDC_USART_H_
#define CDC_USART_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/_stdint.h>

void CDCUSARTInit();
void CDCUSARTPrepareRXData(uint8_t* memoryBaseAddr, uint16_t bufferSize);
void CDCUSARTInitSendData(uint8_t* memoryBaseAddr, uint16_t bufferSize);
void CDCUSARTWaitSendData();
uint16_t CDCUSARTGetRXDataCounter();
void GetCDCUSARTConfig(uint8_t* pBuf);
void SetCDCUSARTConfig(uint8_t* pBuf);

#ifdef __cplusplus
}
#endif

#endif /* CDC_USART_H_ */
