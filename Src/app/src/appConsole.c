/*
 * appConsole.c
 *
 *  Created on: 2019年9月9日
 *      Author: miaodai
 */
#include "appConsole.h"

char ConsoleBackShow;
char gCommandType[12];
char gCommand[64];
char gCommandMsg[256];
u32  gCommandMsgLen;

void APP_Set_BackShow(char BackShow)
{
	ConsoleBackShow = BackShow;
}
void APP_Console_Init()
{
	ConsoleBackShow = 0xFF;
	memset(gCommandType,0,12);
	memset(gCommand,0,64);
	memset(gCommandMsg,0,256);
}
void Cat_ShowVer(void* Msg)
{
	APP_Printf("%sI94100 APM Ver: %s",TopName,DemoVal);
   	APP_Printf("%s",TopName);
}
void Set_AudioCodec(void* Msg)
{
	u8 Reg;
	u8 HighData;
	u8 LowData;
	bool ret = false;
	sscanf(Msg,"0x%x 0x%x 0x%x", (u8*)&Reg,(u8*)&HighData,(u8*)&LowData);
	ret = NAU88C22_WirteReg((Reg<<1)+HighData,LowData);
	if(ret)
		APP_Printf("%sSet_AudioCodec 0x%x 0x%x 0x%x PASS",TopName,Reg,HighData,LowData);
	else
		APP_Printf("%sSet_AudioCodec 0x%x 0x%x 0x%x Fail",TopName,Reg,HighData,LowData);
}
#if UseColsonic
void SubFunction_ColsonicSetPar(ConsoleType _ct,MsgType _mt,void* Msg)
{
	u8 ID;
	Par m_Par;
	u32 ret;
	if(_ct == NS)
	{
#if (__COLSONIC_Model_ == __COLSONIC_NS_)
		if(_mt == t_int)
		{
			sscanf(Msg,"%d %d", (u8*)&ID,(u8*)&m_Par.a);
			ret = Colsonic_ns_set_para(ZKSS_handle,ID,&m_Par.a);
			APP_Printf("%sSet_ColsonicNS %d %d %d",TopName,ret,ID,m_Par.a);
		}
		else
		{
			sscanf(Msg,"%d %f", (u8*)&ID,(u8*)&m_Par.f);
			ret = Colsonic_ns_set_para(ZKSS_handle,ID,&m_Par.f);
			APP_Printf("%sSet_ColsonicNS %d %d %f",TopName,ret,ID,m_Par.f);
		}
#else
		APP_Printf("%sSet_ColsonicNS unsupported ",TopName);
#endif
	}
	else if(_ct == AEC)
	{
#if (__COLSONIC_Model_ == __COLSONIC_AEC_)
		if(_mt == t_int)
		{
			sscanf(Msg,"%d %d", (u8*)&ID,(u8*)&m_Par.a);
			ret = Colsonic_aec_set_para(ZKSS_handle,ID,&m_Par.a);
			APP_Printf("%sSet_ColsonicAEC %d %d %d",TopName,ret,ID,m_Par.a);
		}
		else
		{
			sscanf(Msg,"%d %f", (u8*)&ID,(u8*)&m_Par.f);
			ret = Colsonic_aec_set_para(ZKSS_handle,ID,&m_Par.f);
			APP_Printf("%sSet_ColsonicAEC %d %d %f",TopName,ret,ID,m_Par.f);
		}
#else
		APP_Printf("%sSet_ColsonicAEC unsupported ",TopName);
#endif
	}
	else if(_ct == APM)
	{
#if (__COLSONIC_Model_ == __COLSONIC_AEC_NS_)
		if(_mt == t_int)
		{
			sscanf(Msg,"%d %d", (u8*)&ID,(u8*)&m_Par.a);
			ret = Colsonic_apm_set_para(ZKSS_handle,ID,&m_Par.a);
			APP_Printf("%sSet_ColsonicAPM %d %d %d",TopName,ret,ID,m_Par.a);
		}
		else
		{
			sscanf(Msg,"%d %f", (u8*)&ID,(u8*)&m_Par.f);
			ret = Colsonic_apm_set_para(ZKSS_handle,ID,&m_Par.f);
			APP_Printf("%sSet_ColsonicAPM %d %d %f",TopName,ret,ID,m_Par.f);
		}
#else
		APP_Printf("%sSet_ColsonicAPM unsupported ",TopName);
#endif
	}
	
}
void Set_ColsonicNS(void* Msg)
{
	u8 ID;
	Par m_Par;
	u32 ret;
	sscanf(Msg,"%d %f", (u8*)&ID,(u8*)&m_Par.f);
	if(ID == COLSONIC_NS_DENOISE_LEVEL || 
		ID == COLSONIC_NS_COMPRESSOR_ENABLE ||
		ID == COLSONIC_NS_NOISE_GATE_ENABLE)
		SubFunction_ColsonicSetPar(NS,t_int,Msg);
	//
	else if(ID == COLSONIC_NS_OUTPUT_GAIN ||
		ID == COLSONIC_NS_COMPRESSOR_ALL ||
		ID == COLSONIC_NS_COMPRESSOR_UP_GAIN ||
		ID == COLSONIC_NS_NOISE_GATE_ALL ||
		ID == COLSONIC_NS_NOISE_GATE_THR)
		SubFunction_ColsonicSetPar(NS,t_float,Msg);
	else
		APP_Printf("%sSet_ColsonicNS ID unsupported ",TopName);
		
}
void Set_ColsonicAEC(void* Msg)
{
	u8 ID;
	Par m_Par;
	u32 ret;
	sscanf(Msg,"%d %f", (u8*)&ID,(u8*)&m_Par.f);
	if(ID == COLSONIC_AEC_DENOISE_LEVEL || 
		ID == COLSONIC_AEC_DELAY)
		SubFunction_ColsonicSetPar(AEC,t_int,Msg);
	else
		APP_Printf("%sSet_ColsonicAEC ID unsupported ",TopName);
}
void Set_ColsonicAPM(void* Msg)
{
	u8 ID;
	Par m_Par;
	u32 ret;
	sscanf(Msg,"%d %f", (u8*)&ID,(u8*)&m_Par.f);
	if(ID == COLSONIC_APM_DENOISE_LEVEL || 
		ID == COLSONIC_APM_DELAY ||
		ID == COLSONIC_APM_NS_NOISE_GATE_ENABLE ||
		ID == COLSONIC_APM_AEC_ENABLE ||
		ID == COLSONIC_APM_NS_ENABLE ||
		ID == COLSONIC_APM_NS_COMPRESSOR_ENABLE)
		SubFunction_ColsonicSetPar(APM,t_int,Msg);
	//
	else if(ID == COLSONIC_APM_OUTPUT_GAIN ||
		ID == COLSONIC_APM_NS_COMPRESSOR_ALL ||
		ID == COLSONIC_APM_NS_COMPRESSOR_UP_GAIN ||
		ID == COLSONIC_APM_NS_NOISE_GATE_ALL ||
		ID == COLSONIC_APM_NS_NOISE_GATE_THR)
		SubFunction_ColsonicSetPar(APM,t_float,Msg);
	else
		APP_Printf("%sSet_ColsonicAPM ID unsupported ",TopName);

}
void Set_ColsonicDRC(void* Msg)
{
	APP_Printf("%sSet_ColsonicDRC unsupported ",TopName);
}
#endif
void Set_AudioCodec_DACGain(void* Msg)
{
	u8 _Track;
	float _DBGain;
	u32 ret = 0;
	sscanf(Msg,"%d %f", (u8*)&_Track,(u8*)&_DBGain);
	ret = NAU88C22_SetDACGAIN(_Track,_DBGain);
	if(ret)
		APP_Printf("%sSet_AudioCodec_DACGain %d %f PASS",TopName,_Track,_DBGain);
	else
		APP_Printf("%sSet_AudioCodec_DACGain %d %f Fail",TopName,_Track,_DBGain);

}
void Set_AudioCodec_PGAGain(void* Msg)
{
	u8 _Track;
	float _DBGain;
	u32 ret = 0;
	sscanf(Msg,"%d %f", (u8*)&_Track,(u8*)&_DBGain);
	ret = NAU88C22_SetPGAGAIN(_Track,_DBGain);
	if(ret)
		APP_Printf("%sSet_AudioCodec_PGAGain %d %f PASS",TopName,_Track,_DBGain);
	else
		APP_Printf("%sSet_AudioCodec_PGAGain %d %f Fail",TopName,_Track,_DBGain);
}

ConsoleCommand CatCommand[] =
{
    //CommandType 		Command 		update
    {CommandType_Cat,	"ver",			Cat_ShowVer},
    {CommandType_Set,	"audiocodec",	Set_AudioCodec},
#if UseColsonic
    {CommandType_Set,	"ns",			Set_ColsonicNS},
    {CommandType_Set,	"aec",			Set_ColsonicAEC},
    {CommandType_Set,	"apm",			Set_ColsonicAPM},
    {CommandType_Set,	"drc",			Set_ColsonicDRC},
#endif
    {CommandType_Set,	"dacgain",		Set_AudioCodec_DACGain},
    {CommandType_Set,	"pgagain",		Set_AudioCodec_PGAGain},
};

void APP_Console_Analysis(char* Msg)
{
    if(strcmp(Msg, "\r") == 0)
    {
    	APP_Printf("%s",TopName);
    	return;
    }
	char* pCommandType;
	char* pCommand;
	char* pCommandMsg;
	int i;
	memset(gCommandType,0,12);
	memset(gCommand,0,64);
	memset(gCommandMsg,0,256);
	gCommandMsgLen = ap_uart0.uart_rx_len;
	pCommandType = (char*)gCommandType;
	pCommand = (char*)gCommand;
	pCommandMsg = (char*)gCommandMsg;
	if (strstr(Msg, "cat") != NULL)
		strcpy(pCommandType,CommandType_Cat);
	else if(strstr(Msg, "set") != NULL)
		strcpy(pCommandType,CommandType_Set);
	else if(strstr(Msg, "cmd") != NULL)
		strcpy(pCommandType,CommandType_Cmd);
	else
		goto End;
	//
	sscanf(Msg,"%[^ ] %[^ ]",pCommandType,pCommand);
    //
    if(strcmp(pCommandType, CommandType_Cat) == 0
    		|| strcmp(pCommandType, CommandType_Cmd) == 0)
    	sscanf(Msg,"%[^ ] %[^\r]",pCommandType,pCommand);
    else
    	sscanf(Msg,"%[^ ] %[^ ] %[^\r]",pCommandType,pCommand,pCommandMsg);
    //
    for(i=0;i<(sizeof(CatCommand)/sizeof(CatCommand[0]));i++)
    {
    	if(strcmp(CatCommand[i].CommandType , pCommandType) == 0
    			&& strcmp(CatCommand[i].Command, pCommand) == 0)
    	{
    		CatCommand[i].update(pCommandMsg);
    		return;
    	}
    }
End:
    APP_Printf("%sUnknown command",TopName);
    return;
}
void printf_cbuf(uint16_t* buf, uint32_t len, char* name)
{
    uint32_t i;
    APP_Printf("%s[%d] = {", name, len);
    for (i = 0; i < len; i++)
    {
        if (!(i % 16))
        {
            APP_Printf("\r\n");
        }
        APP_Printf("%04x ", buf[i]);
    }
    APP_Printf("\r\n};\r\n");
}
void printf_cbuf_u8(uint8_t* buf, uint32_t len, char* name)
{
    uint32_t i;
    APP_Printf("%s[%d] = {", name, len);
    for (i = 0; i < len; i++)
    {
        if (!(i % 16))
        {
            APP_Printf("\r\n");
        }
        APP_Printf("%02x ", buf[i]);
    }
    APP_Printf("\r\n};\r\n");
}

