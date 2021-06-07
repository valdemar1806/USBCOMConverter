/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "ext_def.h"
#include "usb_cdc.h"
#include "usb_ll.h"
#include "usb_req.h"

static void EP_ISR_Handler(_USBDevice* device) {
	while (USB->ISTR & USB_ISTR_CTR) {
		uint16_t regISTR = USB->ISTR;

		if (regISTR & USB_ISTR_EP_ID) { //for EPn,n=1...7
			if (regISTR & USB_ISTR_DIR) {
				uint16_t regEPnR = EPGetReg(CDC_OUT_EP & EP_ADDR_MSK);

				if (regEPnR & USB_EP_CTR_RX) {
					OutStage(CDC_OUT_EP & EP_ADDR_MSK, device);
				}
			} else {
				InStage(CDC_IN_EP & EP_ADDR_MSK, device);
			}
		} else { //for EP0 only
			if (regISTR & USB_ISTR_DIR) {
				uint16_t regEPnR = EPGetReg(0);

				if (regEPnR & USB_EP_SETUP) {
					SetupStage(device);
				} else if (regEPnR & USB_EP_CTR_RX) {
					OutStage(0, device);
				}
			} else {
				InStage(0, device);
			}
		}
	}
}

void USB_IRQHandler(_USBDevice* device) {
	if ((USBGetInterrupts() & USB_ISTR_CTR) == USB_ISTR_CTR) {
		EP_ISR_Handler(device);
	}

	if ((USBGetInterrupts() & USB_ISTR_RESET) == USB_ISTR_RESET) {
		_EPoint* ep = &device->OUT_EP[0];
		EPActivate(ep);
		ep = &device->IN_EP[0];
		EPActivate(ep);

		device->dev_state = USBD_STATE_DEFAULT;

		USBSetDADDR(0);

		USBClearPendingInterruptBit(USB_ISTR_RESET);
	}

	if ((USBGetInterrupts() & USB_ISTR_PMAOVR) == USB_ISTR_PMAOVR) {
		USBClearPendingInterruptBit(USB_ISTR_PMAOVR);
	}

	if ((USBGetInterrupts() & USB_ISTR_ERR) == USB_ISTR_ERR) {
		USBClearPendingInterruptBit(USB_ISTR_ERR);
	}

	if ((USBGetInterrupts() & USB_ISTR_WKUP) == USB_ISTR_WKUP) {
		WKUP_ISR_Handler();

		if (device->dev_state == USBD_STATE_SUSPENDED) {
			device->dev_state = device->old_dev_state;
		}
	}

	if ((USBGetInterrupts() & USB_ISTR_SUSP) == USB_ISTR_SUSP) {
		SUSP_ISR_Handler();

		device->old_dev_state = device->dev_state;
		device->dev_state = USBD_STATE_SUSPENDED;
	}

	if ((USBGetInterrupts() & USB_ISTR_SOF) == USB_ISTR_SOF) {
		USBClearPendingInterruptBit(USB_ISTR_SOF);
	}

	if ((USBGetInterrupts() & USB_ISTR_ESOF) == USB_ISTR_ESOF) {
		USBClearPendingInterruptBit(USB_ISTR_ESOF);
	}
}
