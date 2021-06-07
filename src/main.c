/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "hardware.h"
#include "led.h"
#include "usb_device.h"
#include "usb_cdc.h"

_USBDevice USBDevice;

/**
 **===========================================================================
 **
 **  main program
 **
 **===========================================================================
 */
int main(void) {
	SetSysClock();

	LedInit();

	USBDeviceInit(&USBDevice);
	while (1) {
		CDC_Loop(&USBDevice);
	}
}
