/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include "ext_def.h"
#include "usb_def.h"
#include "usb_ll.h"

#define PMA_ACCESS                            (2)
#define USB_CNTRX_NBLK_MSK                    (0x1FU << 10)
#define USB_CNTRX_BLSIZE                      (0x1U << 15)

#define EP_T_MASK (~(USB_EP_DTOG_RX|USB_EPRX_DTOG1|USB_EPRX_DTOG2|USB_EP_DTOG_TX|USB_EPTX_DTOG1|USB_EPTX_DTOG2))

void USBReset() {
	/* CNTR_FRES = 1 */
	USB->CNTR = (uint16_t) USB_CNTR_FRES;

	/* CNTR_FRES = 0 */
	USB->CNTR = 0U;

	/* Clear pending interrupts */
	USB->ISTR = 0U;

	/*Set Btable Address*/
	USB->BTABLE = BTABLE_ADDRESS;
}

void USBEnableInt() {
	USB->ISTR = 0U;
	USB->CNTR = USB_CNTR_CTRM | USB_CNTR_WKUPM |
	USB_CNTR_SUSPM | USB_CNTR_ERRM |
	USB_CNTR_SOFM | USB_CNTR_ESOFM |
	USB_CNTR_RESETM;
}

void USBDisableInt() {
	USB->CNTR &= ~(USB_CNTR_CTRM | USB_CNTR_WKUPM |
	USB_CNTR_SUSPM | USB_CNTR_ERRM |
	USB_CNTR_SOFM | USB_CNTR_ESOFM |
	USB_CNTR_RESETM);
}

uint16_t USBGetInterrupts() {
	return USB->ISTR;
}

void USBClearPendingInterruptBit(uint16_t interrupt) {
	USB->ISTR &= (uint16_t) ~interrupt;
}

void USBSetDADDR(uint8_t address) {
	if (address == 0U) {
		USB->DADDR = (uint16_t) USB_DADDR_EF;
		return;
	}
	USB->DADDR = (uint16_t) USB_DADDR_EF | address;
}

void ReadPMA(uint16_t PMAAddress, uint8_t *bufRX, uint16_t CntrRX) {
	__IO uint16_t * pPMA = (__IO uint16_t *) (USB_PMAADDR
			+ PMAAddress * PMA_ACCESS);

	uint16_t n = CntrRX >> 1;
	for (uint16_t i = 0; i < n; i++) {
		uint16_t v = *pPMA;
		pPMA += 2;
		*bufRX = (uint8_t) (v & 0xFF);
		bufRX++;
		*bufRX = (uint8_t) ((v >> 8) & 0xFF);
		bufRX++;
	}
	if (CntrRX % 2) {
		uint16_t v = *pPMA;
		*bufRX = (uint8_t) (v & 0xFF);
	}
}

void WritePMA(uint16_t PMAAddress, uint8_t *bufTX, uint16_t CntrTX) {
	__IO uint16_t * pPMA = (__IO uint16_t *) (USB_PMAADDR
			+ PMAAddress * PMA_ACCESS);

	uint16_t n = ((uint16_t) CntrTX + 1) >> 1;
	for (uint16_t i = 0; i < n; i++) {
		uint16_t v = *bufTX;
		bufTX++;
		v = v | (*bufTX << 8);
		bufTX++;
		*pPMA = v;
		pPMA += 2;
	}
}

void EPSetReg(uint8_t i, uint16_t v) {
	switch (i) {
	case 0:
		USB->EP0R = v;
		return;
	case 1:
		USB->EP1R = v;
		return;
	case 2:
		USB->EP2R = v;
		return;
	case 3:
		USB->EP3R = v;
		return;
	case 4:
		USB->EP4R = v;
		return;
	case 5:
		USB->EP5R = v;
		return;
	case 6:
		USB->EP6R = v;
		return;
	case 7:
		USB->EP7R = v;
		return;
	}
}

uint16_t EPGetReg(uint8_t i) {
	switch (i) {
	case 0:
		return USB->EP0R;
	case 1:
		return USB->EP1R;
	case 2:
		return USB->EP2R;
	case 3:
		return USB->EP3R;
	case 4:
		return USB->EP4R;
	case 5:
		return USB->EP5R;
	case 6:
		return USB->EP6R;
	case 7:
		return USB->EP7R;
	default:
		return 0;
	}
}

static void EPClearTGLBit(uint8_t i, uint16_t bit) {
	uint16_t v = EPGetReg(i);
//clear bit
	if (v & bit) { //1?
		v &= (uint16_t) EP_T_MASK; //masked other toggled bits
		v |= bit; //toggle(1->0)
		EPSetReg(i, v);
	}
}

static void EPSetTGLBit(uint8_t i, uint16_t bit) {
	uint16_t v = EPGetReg(i);
//set bit
	if (!(v & bit)) { //0?
		v &= (uint16_t) EP_T_MASK; //masked other toggled bits
		v |= bit; //toggle(0->1)
		EPSetReg(i, v);
	}
}

void EPActivate(_EPoint *ep) {
	uint8_t i = ep->EPAddr & EP_ADDR_MSK;

	uint16_t v = EPGetReg(i);
	switch (ep->EPType) {
	case USBD_EP_TYPE_CTRL:
		v |= USB_EP_CONTROL;
		break;

	case USBD_EP_TYPE_BULK:
		v |= USB_EP_BULK;
		break;

	case USBD_EP_TYPE_INTR:
		v |= USB_EP_INTERRUPT;
		break;

	case USBD_EP_TYPE_ISOC:
		v |= USB_EP_ISOCHRONOUS;
		break;
	}
	v = v | USB_EP_CTR_RX | USB_EP_CTR_TX;
	v &= (uint16_t) EP_T_MASK;
	EPSetReg(i, v);

	v = EPGetReg(i);
	v |= ep->EPAddr & USB_EPADDR_FIELD; //set EP address
	v &= (uint16_t) EP_T_MASK;
	EPSetReg(i, v);

	if (ep->isIN) {
		EPSetSingleAddrTX(i, ep->PMAAddress);

		EPSetSingleCntrTX(i, ep->MaxSize);

		EPClearTGLBit(i, USB_EP_DTOG_TX);

		EPTXState(i, USB_EP_TX_NAK);
	} else {
		EPSetSingleAddrRX(i, ep->PMAAddress);

		EPSetSingleCntrRX(i, ep->MaxSize);

		EPClearTGLBit(i, USB_EP_DTOG_RX);

		EPRXState(i, USB_EP_RX_VALID);
	}
}

void EPDeactivate(_EPoint *ep) {
	uint8_t i = ep->EPAddr & EP_ADDR_MSK;

	if (ep->isIN) {
		EPClearTGLBit(i, USB_EP_DTOG_TX);
		EPTXState(i, USB_EP_TX_DIS);
	} else {
		EPClearTGLBit(i, USB_EP_DTOG_RX);
		EPRXState(i, USB_EP_RX_DIS);
	}
}

static void EPSetSingleValue(uint8_t i, uint16_t v, uint8_t offs) {
	__IO uint16_t * pReg = (__IO uint16_t *) (USB_PMAADDR + USB->BTABLE
			+ (i * 8 + offs) * PMA_ACCESS);
	*pReg = v;
}

void EPSetSingleAddrTX(uint8_t i, uint16_t addr) {
	addr = (addr >> 1) << 1;
	EPSetSingleValue(i, addr, 0);
}

void EPSetSingleCntrTX(uint8_t i, uint16_t cntr) {
	cntr &= 0x03FF;
	EPSetSingleValue(i, cntr, 2);
}

void EPSetSingleAddrRX(uint8_t i, uint16_t addr) {
	addr = (addr >> 1) << 1;
	EPSetSingleValue(i, addr, 4);
}

void EPSetSingleCntrRX(uint8_t i, uint16_t cntr) {
	__IO uint16_t * pReg = (__IO uint16_t *) (USB_PMAADDR + USB->BTABLE
			+ (i * 8 + 6) * PMA_ACCESS);
	if (cntr == 0U) {
		*pReg &= (uint16_t) ~USB_CNTRX_NBLK_MSK;
		*pReg |= USB_CNTRX_BLSIZE;
	} else if (cntr <= 62) {
		uint16_t numBloks = cntr >> 1;
		if (cntr & 1) {
			numBloks++;
		}
		*pReg = (uint16_t) (numBloks << 10);
	} else {
		uint16_t numBloks = cntr >> 5;
		if (!(cntr & 0x1F)) {
			numBloks--;
		}
		*pReg = (uint16_t) (numBloks << 10 | USB_CNTRX_BLSIZE);
	}
}

static uint16_t EPGetSingleValue(uint8_t i, uint8_t offs) {
	__IO uint16_t * pReg = (__IO uint16_t *) (USB_PMAADDR + USB->BTABLE
			+ (i * 8 + offs) * PMA_ACCESS);
	return *pReg;
}

uint16_t EPGetSingleAddrTX(uint8_t i) {
	return EPGetSingleValue(i, 0);
}

uint16_t EPGetSingleCntrTX(uint8_t i) {
	return EPGetSingleValue(i, 2) & 0x3FF;
}

uint16_t EPGetSingleAddrRX(uint8_t i) {
	return EPGetSingleValue(i, 4);
}

uint16_t EPGetSingleCntrRX(uint8_t i) {
	return EPGetSingleValue(i, 6) & 0x3FF;
}

void EPTXState(uint8_t i, uint16_t state) {
	/* Configure STAT_TX */
	if (USB_EPTX_DTOG1 & state) {
		EPSetTGLBit(i, USB_EPTX_DTOG1);
	}
	if (USB_EPTX_DTOG2 & state) {
		EPSetTGLBit(i, USB_EPTX_DTOG2);
	}
}

void EPRXState(uint8_t i, uint16_t state) {
	/* Configure STAT_RX */
	if (USB_EPRX_DTOG1 & state) {
		EPSetTGLBit(i, USB_EPRX_DTOG1);
	}
	if (USB_EPRX_DTOG2 & state) {
		EPSetTGLBit(i, USB_EPRX_DTOG2);
	}
}

void EPTXBitClear(uint8_t i) {
	uint16_t v = EPGetReg(i);
	v &= (uint16_t) EP_T_MASK;
	v &= (uint16_t) ~USB_EP_CTR_TX; //clear CTR_TX
	v |= (uint16_t) USB_EP_CTR_RX; //set CTR_RX
	EPSetReg(i, v);
}

void EPRXBitClear(uint8_t i) {
	uint16_t v = EPGetReg(i);
	v &= (uint16_t) EP_T_MASK;
	v &= (uint16_t) ~USB_EP_CTR_RX; //clear CTR_RX
	v |= (uint16_t) USB_EP_CTR_TX; //set CTR_TX
	EPSetReg(i, v);
}

void EPError(uint8_t i) {
	EPTXState(i, USB_EP_TX_STALL);
}

void WKUP_ISR_Handler() {
	USB->CNTR &= (uint16_t) ~USB_CNTR_LP_MODE;
	USB->CNTR &= (uint16_t) ~USB_CNTR_FSUSP;

	USBClearPendingInterruptBit(USB_ISTR_WKUP);
}

void SUSP_ISR_Handler() {
	/* WA: To Clear Wakeup flag if raised with suspend signal */

	uint16_t store_ep[8];

	/* Store Endpoint register */
	for (uint8_t i = 0; i < 8; i++) {
		store_ep[i] = EPGetReg(i);
	}

	/* FORCE RESET */
	USB->CNTR |= (uint16_t) USB_CNTR_FRES;

	/* CLEAR RESET */
	USB->CNTR &= (uint16_t) ~USB_CNTR_FRES;

	/* wait for reset flag in ISTR */
	while ((USB->ISTR & USB_ISTR_RESET) == 0U) {
	}

	/* Clear Reset Flag */
	USBClearPendingInterruptBit(USB_ISTR_RESET);

	/* Restore Registre */
	for (uint8_t i = 0; i < 8; i++) {
		EPSetReg(i, store_ep[i]);
	}

	/* Force low-power mode in the macrocell */
	USB->CNTR |= (uint16_t) USB_CNTR_FSUSP;

	/* clear of the ISTR bit must be done after setting of CNTR_FSUSP */
	USBClearPendingInterruptBit(USB_ISTR_SUSP);

	USB->CNTR |= (uint16_t) USB_CNTR_LP_MODE;
}
