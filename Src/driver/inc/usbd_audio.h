/**************************************************************************//**
 * @file     usbd_uac.h
 * @version  V1.00
 * @brief    I94100 series USB driver header file
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __USBD_UAC_H__
#define __USBD_UAC_H__


/* Audio Interface Subclass Codes */
#define AUDIO_SUBCLASS_UNDEFINED               0x00
#define AUDIO_SUBCLASS_AUDIOCONTROL            0x01
#define AUDIO_SUBCLASS_AUDIOSTREAMING          0x02
#define AUDIO_SUBCLASS_MIDISTREAMING           0x03

/* Audio Interface Protocol Codes */
#define AUDIO_PROTOCOL_UNDEFINED               0x00


/* bmAttributes in Configuration Descriptor */
#define USB_CONFIG_POWERED_MASK                0x40
#define USB_CONFIG_BUS_POWERED                 0x80
#define USB_CONFIG_SELF_POWERED                0xC0
#define USB_CONFIG_REMOTE_WAKEUP               0x20

/* bMaxPower in Configuration Descriptor */
#define USB_CONFIG_POWER_MA(mA)                ((mA)/2)


/*!<Define Audio information */
#define PLAY_RATE_16K       16000 
#define PLAY_RATE_48K       48000     		/* The audo play sampling rate. */
#define PLAY_RATE_96K       96000  
#define PLAY_CHANNELS    	2         		/* Number of channels. */
#define PLAY_RATE      		PLAY_RATE_48K
#define DPWM_BIT_RES		16


/* Microphone */
#define REC_CHANNELS    	2         		/* Number of channels. */
#define REC_RATE         	16000   	  		
#define AMIC_BIT_RES		  16
#define AMIC_DATASIZE_BC	(AMIC_BIT_RES>>3)//2
//carl 20200608
#define REC_MODE_MONO		
#define REC_RATE_16K        16000    /* The record sampling rate. Must be the same with PLAY_RATE */
#define REC_RATE_48K        48000


/* Define Descriptor information */
// Unit ID  
#define REC_FEATURE_UNITID      0x05    
#define PLAY_FEATURE_UNITID     0x06    
/* Define Descriptor information */
#if(PLAY_CHANNELS == 1)
    #define PLAY_CH_CFG     0x1
#endif
#if(PLAY_CHANNELS == 2)
    #define PLAY_CH_CFG     0x3
#endif

#if(REC_CHANNELS == 1)
    #define REC_CH_CFG     0x1
#endif
#if(REC_CHANNELS == 2)
    #define REC_CH_CFG     0x3
#endif
#if(REC_CHANNELS == 4)
    #define REC_CH_CFG     0xF
#endif


#define UAC_REC_CHANNEL_1CH     0x1
#define UAC_REC_CHANNEL_2CH     0x2
#define UAC_REC_CHANNEL_3CH     0x3
#define UAC_REC_CHANNEL_4CH     0x4

//#ifdef FarComeFromUsb
//#define AMIC2PDMA_BUFF_LEN_2CH      (2*(REC_RATE*REC_CHANNELS/1000))//20*(16000*2/1000) = 640  
//#define UAC_BUFFER_LEN				(20*(REC_RATE*REC_CHANNELS/1000))
//#define AMIC_RING_BUFFER_LEN_2CH    (5*UAC_BUFFER_LEN)//10
//#define AMIC_BUFF_UPPER_THRE_2CH	(3*UAC_BUFFER_LEN)//5 
//#define AMIC_BUFF_LOWER_THRE_2CH	(2*UAC_BUFFER_LEN)//3
////
//#define MAX_I2STX_BUFFER_LEN  	 (80*AMIC2PDMA_BUFF_LEN_2CH/2)//(80*PLAY_RATE_16K/1000) 
//#define USB_BUFFER_THRE_BASE     (AMIC2PDMA_BUFF_LEN_2CH/2)//(PLAY_RATE_16K/1000)			
//#define I2STX_BUFFER_UPPER_THRE  (40*USB_BUFFER_THRE_BASE)//(40*USB_BUFFER_THRE_BASE)		 
//#define I2STX_BUFFER_LOWER_THRE  (40*USB_BUFFER_THRE_BASE)//(40*USB_BUFFER_THRE_BASE)	
//#define I2S_TX_PINGPONG_SIZE   	 (USB_BUFFER_THRE_BASE)//(PLAY_RATE_16K/1000)
//#else
#define AMIC2PDMA_BUFF_LEN_2CH      (10*(REC_RATE*REC_CHANNELS/1000))//=320
#define UAC_BUFFER_LEN				(20*(REC_RATE*REC_CHANNELS/1000))
#define AMIC_RING_BUFFER_LEN_2CH    (5*UAC_BUFFER_LEN)//10
#define AMIC_BUFF_UPPER_THRE_2CH	(3*UAC_BUFFER_LEN)//5 
#define AMIC_BUFF_LOWER_THRE_2CH	(2*UAC_BUFFER_LEN)//3
//
#define MAX_I2STX_BUFFER_LEN  	 (30*AMIC2PDMA_BUFF_LEN_2CH/2)//(80*PLAY_RATE_16K/1000) 
#define USB_BUFFER_THRE_BASE     (AMIC2PDMA_BUFF_LEN_2CH/2)//(PLAY_RATE_16K/1000)			
#define I2STX_BUFFER_UPPER_THRE  (20*USB_BUFFER_THRE_BASE)//(40*USB_BUFFER_THRE_BASE)		 
#define I2STX_BUFFER_LOWER_THRE  (10*USB_BUFFER_THRE_BASE)//(40*USB_BUFFER_THRE_BASE)	
#define I2S_TX_PINGPONG_SIZE   	 (USB_BUFFER_THRE_BASE)//(PLAY_RATE_16K/1000)
//#endif
//	
#define RECORD_BYTE_BASE        (REC_RATE*UAC_REC_CHANNEL_2CH*AMIC_DATASIZE_BC/1000)//16000*2*2/1000 = 64	
#define AMIC_RECORD_ADD		    (2*(UAC_REC_CHANNEL_2CH*AMIC_DATASIZE_BC))//2*(2*2) = 8
#define AMIC_RECORD_SUB		    (2*(UAC_REC_CHANNEL_2CH*AMIC_DATASIZE_BC))//2*(2*2) = 8
#define AMIC_EP3BC_ADD		    (2*(UAC_REC_CHANNEL_2CH*AMIC_DATASIZE_BC))//2*(2*2) = 8
#define AMIC_EP3BC_SUB		    (2*(UAC_REC_CHANNEL_2CH*AMIC_DATASIZE_BC))//2*(2*2) = 8

#define USB_BUFFER_EP4_BASE		(0x40000) //>>14  //16 (0xC0000)48k
#define USB_BUFFER_EP4_OFFSET	(0x08000) //2
#define USB_BUFFER_EP4_ADD		(0x08000) //2
#define USB_BUFFER_EP4_SUB		(0x08000) //2 
/********************************************/
/* Audio Class Current State                */
/********************************************/
/*!<Define Audio Class Current State */
#define UAC_STOP_AUDIO_RECORD       	0
#define UAC_START_AUDIO_RECORD      	1
#define UAC_READY_AUDIO_RECORD     		2
#define UAC_BUSY_AUDIO_RECORD       	3

#define UAC_STOP_AUDIO_SPEAK            0
#define UAC_START_AUDIO_SPEAK           1
#define UAC_PROCESS1_AUDIO_SPEAK        2
#define UAC_PROCESS2_AUDIO_SPEAK        3
#define UAC_BUSY_AUDIO_SPEAK            4


/***************************************************/
/*      Audio Class-Specific Request Codes         */
/***************************************************/
/*!<Define Audio Class Specific Request */
#define UAC_REQUEST_CODE_UNDEFINED  0x00
#define UAC_SET_CUR                 0x01
#define UAC_GET_CUR                 0x81
#define UAC_SET_MIN                 0x02
#define UAC_GET_MIN                 0x82
#define UAC_SET_MAX                 0x03
#define UAC_GET_MAX                 0x83
#define UAC_SET_RES                 0x04
#define UAC_GET_RES                 0x84
#define UAC_SET_MEM                 0x05
#define UAC_GET_MEM                 0x85
#define UAC_GET_STAT                0xFF

/*!<Define HID Class Specific Request */
#define HID_SET_REPORT              0x09
#define HID_SET_IDLE                0x0A
#define HID_SET_PROTOCOL            0x0B

#define MUTE_CONTROL                0x01
#define VOLUME_CONTROL              0x02


#endif //__USBD_UAC_H__

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
