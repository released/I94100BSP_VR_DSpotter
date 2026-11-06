
#include "I94100.h"
#include "hwh_i2c.h"



i2c_type ap_i2c;



void i2c0_init(void)
{
	CLK_EnableModuleClock(I2C0_MODULE);
	SYS_ResetModule(I2C0_RST);
	CLK_SetModuleClock(I2C0_MODULE, MODULE_NoMsk, MODULE_NoMsk);
	I2C_Open(I2C0,100000);
	
	I2C0->CTL |= (1<<7);  //使能中断
	I2C0->CTL |= (1<<6);  //时钟使能
	
	SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk)) | (SYS_GPA_MFPH_PA9MFP_I2C0_SCL | SYS_GPA_MFPH_PA10MFP_I2C0_SDA);

	NVIC_ClearPendingIRQ(I2C0_IRQn);
	NVIC_EnableIRQ(I2C0_IRQn);
	I2C_EnableInt(I2C0);
}



void iic0_writer(uint8_t add, uint8_t *date, uint8_t len)
{
	uint8_t ii;
	if(ap_i2c.i2c_tx_len)	return;	
	
	ap_i2c.i2c_tx_per =0;
	ap_i2c.i2c_tx_data[0]=(DEA_ADD<<1);
	ap_i2c.i2c_tx_data[1]=add;
	
	for(ii=0; ii<len; ii++)
	{
		ap_i2c.i2c_tx_data[2+ii]= *date++;
	}
	ap_i2c.i2c_tx_len =len+2;
	I2C_SET_CONTROL_REG(I2C0, I2C_CTL_STA);	//开始

}


void I2C0_IRQHandler(void)
{

	if(I2C_GET_TIMEOUT_FLAG(I2C0))
	{
		I2C_ClearTimeoutFlag(I2C0);
		I2C_Close(I2C0);
	}
	else
	{
		switch(I2C_GET_STATUS(I2C0))
		{			
			case VI2C_REGSTA_M_START:
				I2C_SET_DATA(I2C0,ap_i2c.i2c_tx_data[ap_i2c.i2c_tx_per]);
				ap_i2c.i2c_tx_per++;
				//I2C->CTL |= (1<<3);
				I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
			break;
			case VI2C_REGSTA_M_SLAW_ACK:
				case VI2C_REGSTA_M_TXDATA_ACK:
				if(ap_i2c.i2c_tx_per < ap_i2c.i2c_tx_len)
				{
					I2C_SET_DATA(I2C0,ap_i2c.i2c_tx_data[ap_i2c.i2c_tx_per]);
					ap_i2c.i2c_tx_per++;
					I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);	
				}
				else
				{
					ap_i2c.i2c_tx_per =0;
					ap_i2c.i2c_tx_len =0;
					I2C_SET_CONTROL_REG(I2C0, I2C_CTL_STO|I2C_CTL_SI);
				}
			break;

			case VI2C_REGSTA_M_SLAW_NACK:
				case VI2C_REGSTA_M_SLAR_NACK:					
				I2C_SET_CONTROL_REG(I2C0, I2C_CTL_STA | I2C_CTL_STO | I2C_CTL_SI);
				break;
			case VI2C_REGSTA_M_TXDATA_NACK:
				I2C_SET_CONTROL_REG(I2C0, (I2C_CTL_SI|I2C_CTL_STO));
			break;
			
			
		}
	}
}








