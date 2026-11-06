
#include "I94100.h"
#include "hwh_pwm.h"
#include "stdlib.h"
//PPB0/1/2

pwm_type pwm0_0;
pwm_type pwm0_1;
pwm_type pwm0_2;

uint32_t PWM_u16CNR;
void pwm_init(void)
{
	pwm0_0.Duty =30;		//占空比
	pwm0_0.rate = 2000;	//频率
	pwm0_0.Duty =50;		//占空比
	pwm0_1.rate = 2000;	//频率
	pwm0_0.Duty =80;		//占空比
	pwm0_2.rate = 2000;	//频率
	
	CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL2_PWM0SEL_PCLK0, NULL);

	/* case 2.PWM clock frequency is set double to HCLK: select PWM module clock source as PLL */
	//CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL2_PWM0SEL_PLL, NULL);
	
	/* Enable PWM0 module clock */
	CLK_EnableModuleClock(PWM0_MODULE);
	/*---------------------------------------------------------------------------------------------------------*/

	/* Reset PWM0 module */
	SYS_ResetModule(PWM0_RST);

	/* Set PB multi-function pins for PWM0 Channel0~3 */
	SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk )) | (SYS_GPB_MFPL_PB0MFP_PWM0_CH0 | SYS_GPB_MFPL_PB1MFP_PWM0_CH1 | SYS_GPB_MFPL_PB2MFP_PWM0_CH2 );

//	PWM_ENABLE_COMPLEMENTARY_CHAN_MODE(PWM0, PWM_CHAN0_COMP_OUTPUT | PWM_CHAN2_COMP_OUTPUT);

	// PWM0 channel 0 frequency is 100Hz, duty 30%,
	PWM_ConfigOutputChannel(PWM0, 0, pwm0_0.rate, pwm0_0.Duty);
	SYS_UnlockReg();
	PWM_EnableDeadZone(PWM0, 0, 0);
	SYS_LockReg();

	PWM_ConfigOutputChannel(PWM0,1, pwm0_1.rate, pwm0_1.Duty);
	SYS_UnlockReg();
	PWM_EnableDeadZone(PWM0, 1, 0);
	SYS_LockReg();


	// PWM0 channel 2 frequency is 300Hz, duty 50%
	PWM_ConfigOutputChannel(PWM0, 2, pwm0_2.rate, pwm0_2.Duty);
	SYS_UnlockReg();
	PWM_EnableDeadZone(PWM0, 2, 0);
	SYS_LockReg();

	// Enable output of PWM0 channel 0~3
	PWM_EnableOutput(PWM0, 0x7);

	// Start
	PWM_Start(PWM0, 0x7);
	
	
//	/*---------------------------------------------------------------------------------------------------------*/
//	/* PWM clock frequency configuration                                                                       */
//	/*---------------------------------------------------------------------------------------------------------*/
//	/* PWM clock frequency can be set equal or double to HCLK by choosing case 1 or case 2 */
//	/* case 1.PWM clock frequency is set equal to HCLK: select PWM module clock source as PCLK */
//	CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL2_PWM0SEL_PCLK0, NULL);

//	/* case 2.PWM clock frequency is set double to HCLK: select PWM module clock source as PLL */
//	//CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL2_PWM0SEL_PLL, NULL);
//	
//	/* Enable PWM0 module clock */
//	CLK_EnableModuleClock(PWM0_MODULE);
//	/*---------------------------------------------------------------------------------------------------------*/

//	/* Reset PWM0 module */
//	SYS_ResetModule(PWM0_RST);

//	/* Set PB multi-function pins for PWM0 Channel0~3 */
//	SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk )) | (SYS_GPB_MFPL_PB0MFP_PWM0_CH0 | SYS_GPB_MFPL_PB1MFP_PWM0_CH1 | SYS_GPB_MFPL_PB2MFP_PWM0_CH2 );

////  NVIC_EnableIRQ(PWM0_P0_IRQn);

//	/* Set PWM0 channel 0 and 2 as complementary mode */
//	//PWM_ENABLE_COMPLEMENTARY_CHAN_MODE(PWM0, PWM_CHAN0_COMP_OUTPUT | PWM_CHAN2_COMP_OUTPUT);
//	
//	// PWM0 channel 0 frequency is 100Hz, duty 30%,
//	PWM_ConfigOutputChannel(PWM0, 0, pwm0_0.rate, pwm0_0.Duty);
//	PWM_ConfigOutputChannel(PWM0, 1, pwm0_1.rate, pwm0_1.Duty);
//	PWM_u16CNR = PWM_ConfigOutputChannel(PWM0, 2, pwm0_2.rate, pwm0_2.Duty);
//	
//	PWM_ENABLE_TIMER_SYNC(PWM0, 0x07);
////	PWM0->CTL1 &= (~(7<<24));  //独立模式
////	PWM0->CTL0 &= (~(1<<24)); //取消组，独立
////	PWM0->CNTEN = 0x07;  //使能0/1/2 三个CH 计数
////	PWM0->CTL1 &= (~(7<<16));  //独立模式
////	
////	PWM0->WGCTL1 &= ~(0xFF<<17);
////	PWM0->WGCTL1 |= (0x2A<<17);
////	PWM0->WGCTL1 &= ~(0xFF<<1);
////	PWM0->WGCTL1 |= (0x2A<<1);
//	// Enable output of PWM0 channel 0~3
//	PWM_EnableOutput(PWM0, 0x7);
//	// Start
//	PWM_Start(PWM0, 0x7);
//	
////	PWM_TRIGGER_SYNC_START(PWM0);
//	LED_control();
}


uint8_t PWM0_DUTY(uint8_t ChannelNum,uint8_t DutyCycle)
{
	uint8_t Duty;

		Duty = DutyCycle/2;
	switch(ChannelNum)
	{
		case 0:
			if(Duty == pwm0_0.Duty) break;
			else
			{
				pwm0_0.Duty = Duty;
				PWM_ConfigOutputChannel(PWM0, 0, pwm0_0.rate, Duty);
			}
			
		break;
		case 1:
			if(Duty == pwm0_1.Duty) break;
			else
			{
				pwm0_1.Duty = Duty;
				PWM_ConfigOutputChannel(PWM0, 1, pwm0_1.rate, Duty);
			}
		break;
		case 2:
			if(Duty == pwm0_2.Duty) break;
			else
			{
				pwm0_2.Duty = Duty;
				PWM_ConfigOutputChannel(PWM0, 2, pwm0_2.rate, Duty);
			}
		break;
	}
	return Duty;
}


void LED_control(void)
{

}


void LED_2S_task(void)  //放入1ms中断
{
	static uint16_t led_delay=0, ce=0;
	led_delay++;
	if(led_delay < 1000) return;
	
	led_delay =0;	
	PWM0_DUTY(0,rand()); 
	PWM0_DUTY(1,rand()); 
	PWM0_DUTY(2,rand()); 
	
}


