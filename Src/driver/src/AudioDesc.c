/******************************************************************************
 * @file     descriptors.c
 * @brief    M480 series USBD driver source file
 * @version  1.0.0
 * @date     August, 2018
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
/*!<Includes */
#include <string.h>
#include "I94100.h"
#include "usbd_audio.h"
#include "audioclass.h"

#define WBVAL(x) (x&0xFF), ((x >>8) & 0xFF) 
#define B3VAL(x) (x&0xFF), ((x >>8) & 0xFF), ((x >>16) & 0xFF) 

const uint8_t gu8HidReportDesc[] = 
{
    0x05, 0x0C,      // Usage Page (Consumer)
    0x09, 0x01,      // Usage(Consumer Control)
    0xA1, 0x01,      // Collection(Application )
    0x85, 0x01,      // Report ID (1)
    0x15, 0x00,      // Logical Minimum(0x0 )
    0x25, 0x01,      // Logical Maximum(0x1 )
    0x09, 0xE2,      // Usage(Mute)
    0x09, 0xE9,      // Usage(Volume Increment)
    0x09, 0xEA,      // Usage(Volume Decrement)
    0x75, 0x01,      // Report Size(0x1 )
    0x95, 0x03,      // Report Count(0x3 )
    0x81, 0x02,      // Input(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
    0x75, 0x01,      // Report Size(0x1 )
    0x95, 0x05,      // Report Count(0x5 )
    0x81, 0x03,      // Input(Constant, Variable, Absolute) - Padding 	
    0x09, 0x00,      // Usage(Undefined)
    0x75, 0x08,      // Report Size(0x8 )
    0x95, 0x06,      // Report Count(0x6 )
    0x81, 0x02,      // Input(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
    0xC0            // End Collection
};



/*----------------------------------------------------------------------------*/
/*!<USB Device Descriptor */
const uint8_t gu8DeviceDescriptor[] = 
{
	/*设备描述符*/
    LEN_DEVICE,        /* bLength */
    DESC_DEVICE,       /* bDescriptorType */
	WBVAL(0x0200),     /* bcdUSB */
    0x00,              /* bDeviceClass */
    0x00,              /* bDeviceSubClass */
    0x00,              /* bDeviceProtocol */
    EP0_MAX_PKT_SIZE,  /* bMaxPacketSize0 */
	WBVAL(USBD_VID),   /* idVendor */
	WBVAL(USBD_PID),   /* idProduct */ 
    0x00, 0x00,        /* bcdDevice */
    0x01,              /* iManufacture */
    0x02,              /* iProduct */
    0x03,              /* iSerialNumber */
    0x01               /* bNumConfigurations */
};

#define HID_REPORT_DESC_SIZE  sizeof(gu8HidReportDesc) 
    
#if 0
/*!<USB Configure Descriptor */
const uint8_t gu8ConfigDescriptor[] = 
{
	/*配置描述符*/
    LEN_CONFIG,     				/* bLength */
    DESC_CONFIG,    				/* bDescriptorType */
	WBVAL(203-32-61),               /* wTotalLength *///WBVAL(203-32-61+1+31),               /* wTotalLength */	
    0x02,//0x02           				/* bNumInterfaces */
    0x01,           				/* bConfigurationValue */
    0x00,           				/* iConfiguration */
	USB_CONFIG_BUS_POWERED,			/* bmAttributes */ 
	USB_CONFIG_POWER_MA(100),		/* Max power */
	
    /* Standard AC inteface */
    LEN_INTERFACE,                 /* bLength */
    DESC_INTERFACE,                /* bDescriptorType */
    0x00,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOCONTROL,   /* bInterfaceSubClass:AUDIOCONTROL */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* iInterface */
	/**********************************************************************/
    /* Class-spec AC interface descriptor */
    0x09,//0x09                          /* bLength */
    0x24,                          /* bDescriptorType:CS_INTERFACE */
    0x01,                          /* bDescriptorSubType:HEADER */
    0x00, 0x01,                    /* bcdADC:1.0 */ 
    WBVAL(40),                     /* wTotalLength */
    0x01,                          /* bInCollection */
    0x01,                          /* baInterfaceNr(1) */
    //0x02,                          /* baInterfaceNr(2) */
	
	/* Input for microphone */
    0x0C,                  /* bLength */
    0x24,                  /* bDescriptorType:CS_INTERFACE */
    0x02,                  /* bDescriptorSubType:INPUT_TERMINAL*/
    0x04,            	   /* bTerminalID*/
    0x01, 0x02,            /* wTerminalType: 0x0201 microphone*/
    0x00,                  /* bAssocTerminal*/     
    REC_CHANNELS,          /* bNrChannels*/
    REC_CH_CFG, 0x00,      /* wChannelConfig*/   
    0x00,                  /* iChannelNames*/
    0x00,                  /* iTerminal*/
		
	/* Feature Unit */
    0x0A,                   /* bLength */  
    0x24,               	/* bDescriptorType */
    0x06,               	/* bDescriptorSubType */
    REC_FEATURE_UNITID,     /* bUnitID */
    0x04,            		/* bSourceID */
    0x01,               	/* bControlSize */
    0x01,               	/* bmaControls(0) */
    0x02,               	/* bmaControls(1) */
    0x02,               	/* bmaControls(2) */	
    0x00,               	/* iFeature */
		
	/* Input for microphone */
    0x09,               	/* bLength */
    0x24,               	/* bDescriptorType:CS_INTERFACE */
    0x03,               	/* bDescriptorSubType:OUTPUT_TERMINAL */
    0x02,            		/* bTerminalID */
	WBVAL(0x0101),          /* wTerminalType */
    0x00,               	/* bAssocTerminal */
    REC_FEATURE_UNITID,     /* bSourceID */
    0x00,               	/* iTerminal */
    /*********************************************************************microphone interface*/   
    /* Standard AS interface 1, alternate 0 */
    LEN_INTERFACE,                  /* bLength */
    DESC_INTERFACE,                 /* bDescriptorType */
    0x01,                           /* bInterfaceNumber */
    0x00,                           /* bAlternateSetting */
    0x00,                           /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,         /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOSTREAMING,  /* bInterfaceSubClass:AUDIOSTREAMING */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */

    /* Standard AS interface 1, alternate 1 */
    LEN_INTERFACE,                  /* bLength */
    DESC_INTERFACE,                 /* bDescriptorType */
    0x01,                           /* bInterfaceNumber */
    0x01,                           /* bAlternateSetting */
    0x01,                           /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,         /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOSTREAMING,  /* bInterfaceSubClass:AUDIOSTREAMING */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */

    /* Class-spec AS interface */
    0x07,                           /* bLength */
    0x24,                           /* bDescriptorType:CS_INTERFACE */
    0x01,                           /* bDescriptorSubType:AS_GENERAL */
    0x02,                     		/* bTernimalLink */
    0x01,                           /* bDelay */
	WBVAL(0x0001),                  /* wFormatTag */

    /* Type I format type Descriptor */
    0x0B,                           /* bLength */
    0x24,                           /* bDescriptorType:CS_INTERFACE */
    0x02,                           /* bDescriptorSubType:FORMAT_TYPE */
    0x01,                           /* bFormatType:FORMAT_TYPE_I */
    REC_CHANNELS,               	/* bNrChannels */
    0x02,                           /* bSubFrameSize */
    0x10,                           /* bBitResolution */
    0x01,                           /* bSamFreqType : 0 continuous; 1 discrete */
	B3VAL(REC_RATE_16K),                /* Sample Frequency */
		
    /* Standard AS ISO Audio Data Endpoint */
    0x09,                           /* bLength */
    0x05,                           /* bDescriptorType */  
    (EP2 | EP_INPUT),               /* bEndpointAddress */
    0x05,//0x05                     /* bmAttributes */
	WBVAL(EP2_MAX_PKT_SIZE),        /* wMaxPacketSize */
    0x01,                           /* bInterval*/
    0x00,                           /* bRefresh*/
    0x00,                           /* bSynchAddress*/

    /* Class-spec AS ISO Audio Data endpoint Descriptor */
    0x07,                           /* bLength */
    0x25,                           /* bDescriptorType:CS_ENDPOINT */
    0x01,                           /* bDescriptorSubType:EP_GENERAL */
    0x00,                           /* bmAttributes */
    0x00,                           /* bLockDelayUnits */
    0x00,
    0x00,                     		/* wLockDelay */
};
#elif 1
/*!<USB Configure Descriptor */
const uint8_t gu8ConfigDescriptor[] = 
{
	/*配置描述符*/
    LEN_CONFIG,     				/* bLength */
    DESC_CONFIG,    				/* bDescriptorType */
	WBVAL(203),               /* wTotalLength *///WBVAL(203-32-61+1+31),               /* wTotalLength */	
    0x03,//0x02           				/* bNumInterfaces */
    0x01,           				/* bConfigurationValue */
    0x00,           				/* iConfiguration */
	USB_CONFIG_BUS_POWERED,			/* bmAttributes */ 
	USB_CONFIG_POWER_MA(100),		/* Max power */
	
    /* Standard AC inteface */
    LEN_INTERFACE,                 /* bLength */
    DESC_INTERFACE,                /* bDescriptorType */
    0x00,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOCONTROL,   /* bInterfaceSubClass:AUDIOCONTROL */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* iInterface */
	/**********************************************************************/
    /* Class-spec AC interface descriptor */
    0x0A,//0x09                          /* bLength */
    0x24,                          /* bDescriptorType:CS_INTERFACE */
    0x01,                          /* bDescriptorSubType:HEADER */
    0x00, 0x01,                    /* bcdADC:1.0 */ 
    WBVAL((40+1+31)),                     /* wTotalLength */
    0x02,                          /* bInCollection */
    0x01,                          /* baInterfaceNr(1) */
    0x02,                          /* baInterfaceNr(2) */
	
	/* Input for microphone */
    0x0C,                  /* bLength */
    0x24,                  /* bDescriptorType:CS_INTERFACE */
    0x02,                  /* bDescriptorSubType:INPUT_TERMINAL*/
    0x04,            	   /* bTerminalID*/
    0x01, 0x02,            /* wTerminalType: 0x0201 microphone*/
    0x00,                  /* bAssocTerminal*/     
    REC_CHANNELS,          /* bNrChannels*/
    REC_CH_CFG, 0x00,      /* wChannelConfig*/   
    0x00,                  /* iChannelNames*/
    0x00,                  /* iTerminal*/
		
	/* Feature Unit */
    0x0A,                   /* bLength */  
    0x24,               	/* bDescriptorType */
    0x06,               	/* bDescriptorSubType */
    REC_FEATURE_UNITID,     /* bUnitID */
    0x04,            		/* bSourceID */
    0x01,               	/* bControlSize */
    0x03,               	/* bmaControls(0) */
    0x00,               	/* bmaControls(1) */
    0x00,               	/* bmaControls(2) */	
    0x00,               	/* iFeature */
		
	/* Input for microphone */
    0x09,               	/* bLength */
    0x24,               	/* bDescriptorType:CS_INTERFACE */
    0x03,               	/* bDescriptorSubType:OUTPUT_TERMINAL */
    0x02,            		/* bTerminalID */
	WBVAL(0x0101),          /* wTerminalType */
    0x00,               	/* bAssocTerminal */
    REC_FEATURE_UNITID,     /* bSourceID */
    0x00,               	/* iTerminal */
#if 1
    /* Output for speaker */
    0x0C,               	/* bLength */
    0x24,               	/* bDescriptorType:CS_INTERFACE */
    0x02,               	/* bDescriptorSubType:INPUT_TERMINAL */
    0x01,         				/* bTerminalID */
	WBVAL(0x0101),        /* wTerminalType */
    0x00,               	/* bAssocTerminal */
    PLAY_CHANNELS,      	/* bNrChannels */
    PLAY_CH_CFG, 0x00,  	/* wChannelConfig */
    0x00,               	/* iChannelNames */
    0x00,               	/* iTerminal */

	/* Feature Unit */
    0x0A,                 /* bLength */
    0x24,                 /* bDescriptorType */
    0x06,                 /* bDescriptorSubType */
    PLAY_FEATURE_UNITID,  /* bUnitID */
    0x01,           			/* bSourceID */
    0x01,                 /* bControlSize */
    0x01,                 /* bmaControls(0) */
    0x02,                 /* bmaControls(1) */
    0x02,                 /* bmaControls(2) */
    0x00,                 /* iFeature */
    
	/* Output for speaker */
    0x09,                 /* bLength*/
    0x24,                 /* bDescriptorType:CS_INTERFACE*/
    0x03,                 /* bDescriptorSubType:OUTPUT_TERMINAL*/
    0x03,           			/* bTerminalID*/
	WBVAL(0x0301),        /* wTerminalType: 0x0301 speaker*/
    0x00,                 /* bAssocTerminal*/
    PLAY_FEATURE_UNITID,  /* bSourceID*/
    0x00,                 /* iTerminal*/
#endif
    /*********************************************************************microphone interface*/   
    /* Standard AS interface 1, alternate 0 */
    LEN_INTERFACE,                  /* bLength */
    DESC_INTERFACE,                 /* bDescriptorType */
    0x01,                           /* bInterfaceNumber */
    0x00,                           /* bAlternateSetting */
    0x00,                           /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,         /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOSTREAMING,  /* bInterfaceSubClass:AUDIOSTREAMING */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */

    /* Standard AS interface 1, alternate 1 */
    LEN_INTERFACE,                  /* bLength */
    DESC_INTERFACE,                 /* bDescriptorType */
    0x01,                           /* bInterfaceNumber */
    0x01,                           /* bAlternateSetting */
    0x01,                           /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,         /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOSTREAMING,  /* bInterfaceSubClass:AUDIOSTREAMING */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */

    /* Class-spec AS interface */
    0x07,                           /* bLength */
    0x24,                           /* bDescriptorType:CS_INTERFACE */
    0x01,                           /* bDescriptorSubType:AS_GENERAL */
    0x02,                     		/* bTernimalLink */
    0x01,                           /* bDelay */
	WBVAL(0x0001),                  /* wFormatTag */

    /* Type I format type Descriptor */
    0x0B,                           /* bLength */
    0x24,                           /* bDescriptorType:CS_INTERFACE */
    0x02,                           /* bDescriptorSubType:FORMAT_TYPE */
    0x01,                           /* bFormatType:FORMAT_TYPE_I */
    REC_CHANNELS,               	/* bNrChannels */
    0x02,                           /* bSubFrameSize */
    0x10,                           /* bBitResolution */
    0x01,                           /* bSamFreqType : 0 continuous; 1 discrete */
	B3VAL(REC_RATE_16K),                /* Sample Frequency */
		
    /* Standard AS ISO Audio Data Endpoint */
    0x09,                           /* bLength */
    0x05,                           /* bDescriptorType */  
    (EP2 | EP_INPUT),               /* bEndpointAddress */
    0x0D,//0x05                     /* bmAttributes */
	WBVAL(EP2_MAX_PKT_SIZE),        /* wMaxPacketSize */
    0x01,                           /* bInterval*/
    0x00,                           /* bRefresh*/
    0x00,                           /* bSynchAddress*/

    /* Class-spec AS ISO Audio Data endpoint Descriptor */
    0x07,                           /* bLength */
    0x25,                           /* bDescriptorType:CS_ENDPOINT */
    0x01,                           /* bDescriptorSubType:EP_GENERAL */
    0x00,                           /* bmAttributes */
    0x00,                           /* bLockDelayUnits */
    0x00,
    0x00,                     		/* wLockDelay */
#if 1
    /*********************************************************************microphone interface*/   
	/* Standard AS interface 2, alternate 0 */
    LEN_INTERFACE,                  /* bLength */
    DESC_INTERFACE,                 /* bDescriptorType */
    0x02,                           /* bInterfaceNumber */
    0x00,                           /* bAlternateSetting */
    0x00,                           /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,         /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOSTREAMING,  /* bInterfaceSubClass:AUDIOSTREAMING */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */

    /* Standard AS interface 2, alternate 1 */
    LEN_INTERFACE,                  /* bLength */
    DESC_INTERFACE,                 /* bDescriptorType */
    0x02,                           /* bInterfaceNumber */
    0x01,                           /* bAlternateSetting */
    0x02,                           /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,         /* bInterfaceClass:AUDIO */
    AUDIO_SUBCLASS_AUDIOSTREAMING,  /* bInterfaceSubClass:AUDIOSTREAMING */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */

    /* Class-spec AS interface */
    0x07,                           /* bLength */
    0x24,                           /* bDescriptorType:CS_INTERFACE */
    0x01,                           /* bDescriptorSubType:AS_GENERAL */
    0x01,                     		/* bTernimalLink */
    0x01,                           /* bDelay */
	WBVAL(0x0001),                  /* wFormatTag */

    /* Type I format type Descriptor */
    0x0B,                     			/* bLength */
    0x24,                           /* bDescriptorType:CS_INTERFACE */
    0x02,                           /* bDescriptorSubType:FORMAT_TYPE */
    0x01,                           /* bFormatType:FORMAT_TYPE_I */
    PLAY_CHANNELS,                  /* bNrChannels */
    0x02,                           /* bSubFrameSize */
    0x10,                           /* bBitResolution */
    0x01,                           /* bSamFreqType : 0 continuous; 1 discrete */
	B3VAL(PLAY_RATE_16K),			/* Sample Frequency */	   // edit by darren


    /* Standard AS ISO Audio Data Endpoint */
    0x09,                       		/* bLength */
    0x05,                       		/* bDescriptorType */
    EP3 | EP_OUTPUT, 					/* bEndpointAddress */
	0x05,								/* bmAttributes */ 
    WBVAL(EP3_MAX_PKT_SIZE),    		/* wMaxPacketSize */
    0x01,                       		/* bInterval */
    0x00,                       		/* bRefresh */
    0x84,                       		/* bSynchAddress */


    /* Class-spec AS ISO Audio Data endpoint Descriptor */
    0x07,           					/* bLength */
    0x25,           					/* bDescriptorType:CS_ENDPOINT */
    0x01,           					/* bDescriptorSubType:EP_GENERAL */
	0x01,           					/* bmAttributes */
    0x00,           					/* bLockDelayUnits */
    0x00, 
    0x00,     							/* wLockDelay */


	0x09, 								/* bLength */ 
	0x05,								/* bDescriptorType */
	(EP4 | EP_INPUT),   			    /* bEndpointAddress */
    0x11,								/* bmAttributes */
    WBVAL(EP4_MAX_PKT_SIZE),  			/* wMaxPacketSize */
	0x01,								/* bInterval */
	0x05,								/* bRefresh */	  
	0x00,								/* bSynchAddress */

//---------- AudioStreaming Interface Descriptors and Endpoint Descriptors END----------

    /* I/F descr: HID */
//    LEN_INTERFACE,                      /* bLength */
//    DESC_INTERFACE,                     /* bDescriptorType */
//    0x03,   							/* bInterfaceNumber */ 
//    0x00,                               /* bAlternateSetting */
//    0x01,                               /* bNumEndpoints */
//    USB_DEVICE_CLASS_HUMAN_INTERFACE,   /* bInterfaceClass : HID */
//    0x00,                               /* bInterfaceSubClass */
//    0x00,                               /* bInterfaceProtocol */
//    0x00,                               /* iInterface */

//    // HID Descriptor
//    LEN_HID,        					/* Size of this descriptor in UINT8s. */
//    DESC_HID,       			        /* HID descriptor type. */
//	WBVAL(0x0110),                      /* HID Class Spec. release number. */
//    0x00,           			        /* H/W target country. */
//    0x01,           			        /* Number of HID class descriptors to follow. */
//    DESC_HID_RPT,   			        /* Dscriptor type. */
//	WBVAL(HID_REPORT_DESC_SIZE),	/* Total length of report descriptor */
//		
//    /* EP Descriptor: interrupt in */
//    LEN_ENDPOINT,   					/* bLength */
//    DESC_ENDPOINT,  					/* bDescriptorType */
//    (EP5 | EP_INPUT),         			/* bEndpointAddress */
//    EP_INT,         					/* bmAttributes */   
//	WBVAL(EP5_MAX_PKT_SIZE), 			/* wMaxPacketSize */
//    1,              					/* bInterval */
#endif
};
#endif

/*!<USB Language String Descriptor */
const uint8_t gu8StringLang[4] = 
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
const uint8_t gu8VendorStringDesc[16] = 
{
    16,
    DESC_STRING,
    'N', 0, 
    'u', 0, 
    'v', 0, 
    'o', 0, 
    't', 0, 
    'o', 0, 
    'n', 0
};

/*!<USB Product String Descriptor */
const uint8_t gu8ProductStringDesc[] = {
    22,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 
	'A', 0, 
	'C', 0, 
    ' ', 0,
	'D', 0, 
	'e', 0, 
	'v', 0, 
	'i', 0, 
	'c', 0, 
	'e', 0
};

uint8_t gu8StringSerial[26] = 
{
    26,             // bLength
    DESC_STRING,    // bDescriptorType
    'A', 0, 
    '0', 0, 
    '0', 0, 
    '0', 0, 
    '0', 0, 
    '8', 0, 
    '0', 0, 
    '4', 0, 
    '0', 0, 
    '1', 0, 
    '1', 0, 
    '5', 0
};

/*!<USB BOS Descriptor */
const uint8_t gu8BOSDescriptor[] = 
{
    LEN_BOS,        /* bLength */
    DESC_BOS,       /* bDescriptorType */
    /* wTotalLength */
    0x0C & 0x00FF,
    (0x0C & 0xFF00) >> 8,
    0x01,           /* bNumDeviceCaps */

    /* Device Capability */
    0x7,            /* bLength */
    DESC_CAPABILITY,/* bDescriptorType */
    CAP_USB20_EXT,  /* bDevCapabilityType */
    0x02, 0x00, 0x00, 0x00  /* bmAttributes */
};

const uint8_t *gpu8UsbString[4] = {
    gu8StringLang,
    gu8VendorStringDesc,
    gu8ProductStringDesc,
    gu8StringSerial
};

const uint8_t *gu8UsbHidReport[6] = {
    NULL,
    NULL,
    NULL,
    gu8HidReportDesc,	
	NULL,
	NULL 
};

const uint32_t gu32UsbHidReportLen[6] = {
    0,
    0,
    0,
    sizeof(gu8HidReportDesc),
    0,
    0
};

const uint32_t gu32ConfigHidDescIdx[4] = {
    0,
    0,
    0,
    (sizeof(gu8ConfigDescriptor) - LEN_ENDPOINT - LEN_HID)
};

const S_USBD_INFO_T gsInfo = 
{
    (uint8_t *)gu8DeviceDescriptor,
    (uint8_t *)gu8ConfigDescriptor,
    (const uint8_t **)gpu8UsbString,
    (const uint8_t **)gu8UsbHidReport,
    (uint8_t *)gu8BOSDescriptor,
    (uint32_t *)gu32UsbHidReportLen,
    (uint32_t *)gu32ConfigHidDescIdx
};
