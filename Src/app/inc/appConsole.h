/*
 * appConsole.h
 *
 *  Created on: 2019年9月9日
 *      Author: miaodai
 */
#ifndef APPCONSOLE_H_
#define APPCONSOLE_H_
#include "c_stdfx.h"

//#define TopName "\r\nColsonic:~#"
//Command line
#define CommandType_Set "set"//设置
#define CommandType_Cat "cat"//读取
#define CommandType_Cmd "cmd"//命令

typedef struct
{
    char* CommandType;
    char* Command;
    void ( *update ) ( void * );
} ConsoleCommand;
typedef union{
	int a;
	float f;
}Par;
typedef enum
{
    NS = 1,
    AEC,
    APM,
    DRC,
} ConsoleType;
typedef enum
{
    t_int = 1,
	t_float,
} MsgType;
//

extern char ConsoleBackShow;
extern void APP_Console_Init(void);
extern void APP_Console_Analysis(char* Msg);
extern void APP_Set_BackShow(char BackShow);
extern void printf_cbuf(uint16_t* buf, uint32_t len, char* name);
extern void printf_cbuf_u8(uint8_t* buf, uint32_t len, char* name);
#endif /* APPCONSOLE_H_ */
