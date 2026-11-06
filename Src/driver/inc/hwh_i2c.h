
#ifndef _I2C_H_
#define _I2C_H_	  

#include "stdint.h"





#define VI2C_REGSTA_M_START             (0x08)  // Start has been transmitted.
#define VI2C_REGSTA_M_SLAW_ACK          (0x18)  // SLA+W has been transmitted. ACK bit will be received.
#define VI2C_REGSTA_M_SLAW_NACK         (0x20)  // SLW+W has been transmitted. NACK bit will be received.
#define VI2C_REGSTA_M_TXDATA_ACK        (0x28)  // Data has been transmitted. ACK bit will be received.
#define VI2C_REGSTA_M_TXDATA_NACK       (0x30)  // Data has been transmitted. NACK bit will be received.
#define VI2C_REGSTA_M_SLAR_ACK          (0x40)  // SLA+R has been transmitted. ACK has been received.
#define VI2C_REGSTA_M_SLAR_NACK         (0x48)  // SLA+R has been transmitted. NACK has been received.
#define VI2C_REGSTA_M_RXDATA_ACK        (0x50)  // Data has been transmitted. ACK has been received.
#define VI2C_REGSTA_M_RXDATA_NACK       (0x58)  // Data has been transmitted. NACK has been received.

#define DEA_ADD	0x40  //设备器件地址

#define I2C0_MASTER_PINS_MSK    (SYS_GPA_MFP_PA6MFP_Msk|SYS_GPA_MFP_PA7MFP_Msk)
#define I2C0_MASTER_PINS        (SYS_GPA_MFP_PA6MFP_I2C0_SDA|SYS_GPA_MFP_PA7MFP_I2C0_SCL)	



#define I2C_DA		10 
typedef struct		//收发数据结构
{

uint8_t i2c_tx_data[I2C_DA];
uint8_t	i2c_rx_data[I2C_DA];
	
uint8_t i2c_tx_per;
uint8_t	i2c_tx_len;
uint8_t	i2c_rx_len;
	

}i2c_type;



void i2c0_init(void);
void iic0_writer(uint8_t add, uint8_t *date, uint8_t len);











#endif










