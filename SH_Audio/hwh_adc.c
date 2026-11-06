
#include "hwh_adc.h"
#include "I94100.h"

uint16_t ADC_DATA[2];

void ADC_init(void)
{
			CLK_EnableModuleClock(EADC_MODULE);

			/* EADC clock source is PCLK1 = 72MHz/2, set divider to 8, EADC clock is 36/8 MHz */
			CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(8));

		SYS->GPA_MFPL &= ~ SYS_GPA_MFPL_PA7MFP_Msk;
		SYS->GPA_MFPL |= SYS_GPA_MFPL_PA7MFP_EADC0_CH7;
		SYS->GPA_MFPH &= ~ SYS_GPA_MFPH_PA8MFP_Msk;
		SYS->GPA_MFPH |= SYS_GPA_MFPH_PA8MFP_EADC0_CH8;
	
		SYS_ResetModule(EADC_RST);
    EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);
		
	EADC_SetTriggerDelayTime(EADC,7,0x10,EADC_SCTL_TRGDLYDIV_DIVIDER_4);
	EADC_SetTriggerDelayTime(EADC,8,0x10,EADC_SCTL_TRGDLYDIV_DIVIDER_4);
	
		/* Clear the A/D ADINT0 interrupt flag for safe */
		EADC_CLR_INT_FLAG(EADC, 0x1);

		/* Enable the sample module 0 interrupt */
		EADC_ENABLE_INT(EADC, 0x1);//Enable sample module A/D ADINT0 interrupt.
		EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, 0x1);//Enable sample module 0 interrupt.
		NVIC_ClearPendingIRQ(EADC0_IRQn);
		NVIC_EnableIRQ(EADC0_IRQn);
		
		EADC_CONV_RESET(EADC);
	EADC_START_CONV(EADC,7);  //选择通道
}


void EADC0_IRQHandler(void)
{
	uint32_t sta;
    EADC_CLR_INT_FLAG(EADC, 0x1);      /* Clear the A/D ADINT0 interrupt flag */
	sta = 	EADC->OVSTS &0xfff;
	
	if(sta&(1<<7))
	{
		ADC_DATA[0] = EADC->DAT[7]&0xfff;
		EADC_START_CONV(EADC,8);  //启动
	}
	else
	{
		ADC_DATA[1] = EADC->DAT[8]&0xfff;
		EADC_START_CONV(EADC,7);  //启动
	}
}





