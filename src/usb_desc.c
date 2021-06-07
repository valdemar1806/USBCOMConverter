/*
 * MIT License
 *
 * Copyright (c) 2021 IE Vladimir Kabatzcky, Novosibirsk
 */

#include <stddef.h>
#include "usb_cdc.h"

#define  USB_LEN_DEV_QUALIFIER_DESC                     0x0AU
#define  USB_LEN_DEV_DESC                               0x12U
#define  USB_LEN_CFG_DESC                               0x09U
#define  USB_LEN_IF_DESC                                0x09U
#define  USB_LEN_EP_DESC                                0x07U
#define  USB_LEN_OTG_DESC                               0x03U
#define  USB_LEN_LANGID_STR_DESC                        0x04U
#define  USB_LEN_OTHER_SPEED_DESC_SIZ                   0x09U

#define  USB_DESC_TYPE_DEVICE                           0x01U
#define  USB_DESC_TYPE_CONFIGURATION                    0x02U
#define  USB_DESC_TYPE_STRING                           0x03U
#define  USB_DESC_TYPE_INTERFACE                        0x04U
#define  USB_DESC_TYPE_ENDPOINT                         0x05U
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                 0x06U
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION        0x07U
#define  USB_DESC_TYPE_BOS                              0x0FU

#define  USBD_IDX_LANGID_STR                            0x00U
#define  USBD_IDX_MFC_STR                               0x01U
#define  USBD_IDX_PRODUCT_STR                           0x02U
#define  USBD_IDX_SERIAL_STR                            0x03U
#define  USBD_IDX_CONFIG_STR                            0x04U
#define  USBD_IDX_INTERFACE_STR                         0x05U

#define USBD_MAX_STR_DESC_SIZ     512

#define UID_BASE              0x1FFFF7E8UL    /*!< Unique device ID register base address */

#define DEVICE_ID1          (UID_BASE)
#define DEVICE_ID2          (UID_BASE + 0x4)
#define DEVICE_ID3          (UID_BASE + 0x8)

#define USB_SIZ_STRING_SERIAL       0x1A

#define USBD_VID                       0x03EB
#define USBD_LANGID_STRING             1033
#define USBD_MANUFACTURER_STRING       "FG"
#define USBD_PID_FS                    0x6119
#define USBD_PRODUCT_STRING_FS         "FG Virtual ComPort"
#define USBD_CONFIGURATION_STRING_FS   "CDC Config"
#define USBD_INTERFACE_STRING_FS       "CDC Interface"

/* USB Standard Device Descriptor */
static uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] = {
USB_LEN_DEV_QUALIFIER_DESC,
USB_DESC_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00 };

static uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] = { 0x12, /*bLength */
USB_DESC_TYPE_DEVICE, /*bDescriptorType*/
0x00, /*bcdUSB */
0x02, 0x02, /*bDeviceClass*/
0x02, /*bDeviceSubClass*/
0x00, /*bDeviceProtocol*/
USB_MAX_EP0_SIZE, /*bMaxPacketSize*/
LOBYTE(USBD_VID), /*idVendor*/
HIBYTE(USBD_VID), /*idVendor*/
LOBYTE(USBD_PID_FS), /*idProduct*/
HIBYTE(USBD_PID_FS), /*idProduct*/
0x00, /*bcdDevice rel. 2.00*/
0x02, USBD_IDX_MFC_STR, /*Index of manufacturer  string*/
USBD_IDX_PRODUCT_STR, /*Index of product string*/
USBD_IDX_SERIAL_STR, /*Index of serial number string*/
USBD_MAX_NUM_CONFIGURATION /*bNumConfigurations*/
};

/* USB CDC device Configuration Descriptor */
static uint8_t USBD_CDC_CfgFSDesc[USB_CDC_CONFIG_DESC_SIZ] = {
/*Configuration Descriptor*/
0x09, /* bLength: Configuration Descriptor size */
USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType: Configuration */
USB_CDC_CONFIG_DESC_SIZ, /* wTotalLength:no of returned bytes */
0x00, 0x02, /* bNumInterfaces: 2 interface */
0x01, /* bConfigurationValue: Configuration value */
0x00, /* iConfiguration: Index of string descriptor describing the configuration */
0xC0, /* bmAttributes: self powered */
0x32, /* MaxPower 0 mA */

/*---------------------------------------------------------------------------*/

/*Interface Descriptor */
0x09, /* bLength: Interface Descriptor size */
USB_DESC_TYPE_INTERFACE, /* bDescriptorType: Interface */
/* Interface descriptor type */
0x00, /* bInterfaceNumber: Number of Interface */
0x00, /* bAlternateSetting: Alternate setting */
0x01, /* bNumEndpoints: One endpoints used */
0x02, /* bInterfaceClass: Communication Interface Class */
0x02, /* bInterfaceSubClass: Abstract Control Model */
0x01, /* bInterfaceProtocol: Common AT commands */
0x00, /* iInterface: */

/*Header Functional Descriptor*/
0x05, /* bLength: Endpoint Descriptor size */
0x24, /* bDescriptorType: CS_INTERFACE */
0x00, /* bDescriptorSubtype: Header Func Desc */
0x10, /* bcdCDC: spec release number */
0x01,

/*Call Management Functional Descriptor*/
0x05, /* bFunctionLength */
0x24, /* bDescriptorType: CS_INTERFACE */
0x01, /* bDescriptorSubtype: Call Management Func Desc */
0x00, /* bmCapabilities: D0+D1 */
0x01, /* bDataInterface: 1 */

/*ACM Functional Descriptor*/
0x04, /* bFunctionLength */
0x24, /* bDescriptorType: CS_INTERFACE */
0x02, /* bDescriptorSubtype: Abstract Control Management desc */
0x02, /* bmCapabilities */

/*Union Functional Descriptor*/
0x05, /* bFunctionLength */
0x24, /* bDescriptorType: CS_INTERFACE */
0x06, /* bDescriptorSubtype: Union func desc */
0x00, /* bMasterInterface: Communication class interface */
0x01, /* bSlaveInterface0: Data Class Interface */

/*Endpoint 2 Descriptor*/
0x07, /* bLength: Endpoint Descriptor size */
USB_DESC_TYPE_ENDPOINT, /* bDescriptorType: Endpoint */
CDC_CMD_EP, /* bEndpointAddress */
0x03, /* bmAttributes: Interrupt */
LOBYTE(CDC_CMD_PACKET_SIZE), /* wMaxPacketSize: */
HIBYTE(CDC_CMD_PACKET_SIZE), CDC_FS_BINTERVAL, /* bInterval: */
/*---------------------------------------------------------------------------*/

/*Data class interface descriptor*/
0x09, /* bLength: Endpoint Descriptor size */
USB_DESC_TYPE_INTERFACE, /* bDescriptorType: */
0x01, /* bInterfaceNumber: Number of Interface */
0x00, /* bAlternateSetting: Alternate setting */
0x02, /* bNumEndpoints: Two endpoints used */
0x0A, /* bInterfaceClass: CDC */
0x00, /* bInterfaceSubClass: */
0x00, /* bInterfaceProtocol: */
0x00, /* iInterface: */

/*Endpoint OUT Descriptor*/
0x07, /* bLength: Endpoint Descriptor size */
USB_DESC_TYPE_ENDPOINT, /* bDescriptorType: Endpoint */
CDC_OUT_EP, /* bEndpointAddress */
0x02, /* bmAttributes: Bulk */
LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), /* wMaxPacketSize: */
HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), 0x00, /* bInterval: ignore for Bulk transfer */

/*Endpoint IN Descriptor*/
0x07, /* bLength: Endpoint Descriptor size */
USB_DESC_TYPE_ENDPOINT, /* bDescriptorType: Endpoint */
CDC_IN_EP, /* bEndpointAddress */
0x02, /* bmAttributes: Bulk */
LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), /* wMaxPacketSize: */
HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), 0x00 /* bInterval: ignore for Bulk transfer */
};

static uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] = {
USB_LEN_LANGID_STR_DESC,
USB_DESC_TYPE_STRING, LOBYTE(USBD_LANGID_STRING), HIBYTE(USBD_LANGID_STRING) };

static uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] = {
USB_SIZ_STRING_SERIAL,
USB_DESC_TYPE_STRING, };

static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ];

static uint8_t USBD_GetLen(uint8_t *buf) {
	uint8_t len = 0U;

	while (*buf != '\0') {
		len++;
		buf++;
	}

	return len;
}

static void USBD_GetString(uint8_t *desc, uint8_t *unicode, uint16_t *len) {
	uint8_t idx = 0U;

	if (desc != NULL) {
		*len = (uint16_t) USBD_GetLen(desc) * 2U + 2U;
		unicode[idx++] = *(uint8_t *) (void *) len;
		unicode[idx++] = USB_DESC_TYPE_STRING;

		while (*desc != '\0') {
			unicode[idx++] = *desc++;
			unicode[idx++] = 0U;
		}
	}
}

static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len) {
	uint8_t idx = 0;

	for (idx = 0; idx < len; idx++) {
		if (((value >> 28)) < 0xA) {
			pbuf[2 * idx] = (value >> 28) + '0';
		} else {
			pbuf[2 * idx] = (value >> 28) + 'A' - 10;
		}

		value = value << 4;

		pbuf[2 * idx + 1] = 0;
	}
}

static void Get_SerialNum(void) {
	uint32_t deviceserial0, deviceserial1, deviceserial2;

	deviceserial0 = *(uint32_t *) DEVICE_ID1;
	deviceserial1 = *(uint32_t *) DEVICE_ID2;
	deviceserial2 = *(uint32_t *) DEVICE_ID3;

	deviceserial0 += deviceserial2;

	if (deviceserial0 != 0) {
		IntToUnicode(deviceserial0, &USBD_StringSerial[2], 8);
		IntToUnicode(deviceserial1, &USBD_StringSerial[18], 4);
	}
}

uint8_t * GetDeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	*length = sizeof(USBD_FS_DeviceDesc);
	return USBD_FS_DeviceDesc;
}

uint8_t * GetLangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	*length = sizeof(USBD_LangIDDesc);
	return USBD_LangIDDesc;
}

uint8_t * GetProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	USBD_GetString((uint8_t *) USBD_PRODUCT_STRING_FS, USBD_StrDesc, length);
	return USBD_StrDesc;
}

uint8_t * GetManufacturerStrDescriptor(USBD_SpeedTypeDef speed,
		uint16_t *length) {
	USBD_GetString((uint8_t *) USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}

uint8_t * GetSerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	*length = USB_SIZ_STRING_SERIAL;
	Get_SerialNum();
	return (uint8_t *) USBD_StringSerial;
}

uint8_t * GetConfigurationStrDescriptor(USBD_SpeedTypeDef speed,
		uint16_t *length) {
	USBD_GetString((uint8_t *) USBD_CONFIGURATION_STRING_FS, USBD_StrDesc,
			length);
	return USBD_StrDesc;
}

uint8_t * GetInterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	USBD_GetString((uint8_t *) USBD_INTERFACE_STRING_FS, USBD_StrDesc, length);
	return USBD_StrDesc;
}

uint8_t * GetFSConfigDescriptor(uint16_t *length) {
	*length = sizeof(USBD_CDC_CfgFSDesc);
	return USBD_CDC_CfgFSDesc;
}

uint8_t * GetDeviceQualifierDescriptor(uint16_t *length) {
	*length = sizeof(USBD_CDC_DeviceQualifierDesc);
	return USBD_CDC_DeviceQualifierDesc;
}
