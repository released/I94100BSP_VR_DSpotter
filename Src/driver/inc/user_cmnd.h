/******************************************************************************
 * @file     cmnd_list.h
 * @brief    I94100 series USB HID transfer header file
 *
 * @note
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __CMND_LIST_H__
#define __CMND_LIST_H__




#define SPI_LOOPBACK    0
#define DEBUG_ENABLE    1


////////////////////////////////////////////////////////////////////////

#define HID_IDLE             0       /* Idle state */
#define HID_DATA_OUT         1       /* Data Out state */
#define HID_DATA_IN          2       /* Data In state */
#define HID_DATA_IN_LAST     3       /* Last Data In Last */
#define HID_DATA_OUT_FSK     4       /* For FSKSend command */ 
#define HID_IDLE_LPK         5       /* For USB fast test */

//---------------------------------------------------
//Command List
//---------------------------------------------------
//#define FPGA_PROG_START	    0x07
//#define FPGA_PROG_SEND	    0x0b
//#define FPGA_PROG_STOP	    0x09
// #define RESET_XILINX		    0x05
// #define GET_XILINX_SIGNATURE	0x15

//USB IN/OUT fast test 
#define USB_LOOPBACK    0x81

//Need it
#define SELECT_CHANNEL       0xfb
#define WRITE_PROX           0xf9  
#define READ_PROX            0xfa   
#define WRITE_PROX2          0xd0    
#define READ_PROX2           0xd1    
#define FSK_SEND             0xf8
#define BURST_READ_PROX      0xf7
#define BURST_WRITE_PROX     0xf6

//No need
#define ENABLE_INTS         0xf5
#define POLL_INTS           0xf4
#define DISABLE_INTS        0xf3

//Need it ?
#define PX_PWR_ON           0xf2
#define PX_PWR_OFF          0xf1
#define PX_RST_HI           0xf0
#define PX_RST_LO           0xef  
#define PX_TEST_HI          0xee
#define PX_TEST_LO          0xed 

#define PCM_CONFIGURE       0xec
#define CALIBRATION         0xeb

//Need it?
#define READ_BOARD_ID       0xea
#define READ_PROX256        0xe9
#define READ_FW_VERSION     0xe8
#define FAIL0	            0x57
#define FAIL1	            0x46


/* ===== Macro Definition ===== */
#define SPI_RST_TX(port) ((port)->FIFOCTL |= SPI_FIFOCTL_TXRST_Msk)
#define SPI_RST_RX(port) ((port)->FIFOCTL |= SPI_FIFOCTL_RXRST_Msk)


#endif  

/*** (C) COPYRIGHT 2019 Nuvoton Technology Corp. ***/
