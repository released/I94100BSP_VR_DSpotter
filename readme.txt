/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) Nuvoton Technology Corp. All rights reserved.                                              */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
---------------------------------------------------------------------------------------------------------
Purpose:
---------------------------------------------------------------------------------------------------------
Demonstrate voice recognition
	(1) Voice recognition result is shown by Voice Prompt. User needs to connect speaker for verification.
	(2) Output MIC data through UAC.
		
---------------------------------------------------------------------------------------------------------
Operation:
---------------------------------------------------------------------------------------------------------
Programming:
	Can use ICP and import VR_DSpotter22224_88C22_UAC_NuOne.icp file. It do as below:
	1. Config DataFlash from 0x15000 by ICP
	2. Program APROM by Keil or ICP with \Prj\KEIL\obj\DNN_NuOne_UAC.bin.
	3. Program DataFlash by ICP with \CMDSet\Cinderella_pack.bin
  	4. Program Ext Flash by ICP with \AudioRes\Output\AudioRes_AudioInfoMerge.ROM.

Voice recognition:
  This example is a 2-stage voice recognition, user must say key word first. 
  After key word is recognized(D4 is on), user can say the commands.
  Command stage have 10 second to recognition.
  If without success recognized command, it will come back to stage1.(LED D4 will be Off)

    Key word  "Cinderella"     -> Voice Prompt "Ding", and LED D4 will be on.
    Command   "Come in please" -> Voice Prompt "Command1".
    	      "Welcome"        -> Voice Prompt "Command2".
              "Here you are"   -> Voice Prompt "Command3".
              "Go with me"     -> Voice Prompt "Command4".

UAC:
  Connect USB(CN1) to PC and use any recording software to record the sound.
  Sound format is stereo 16KHz sample rate.


Panasonic 冷氣國台語應用程式 V005

主要操作手冊：README.md。
發布與燒錄說明：CMDSet/ReleaseV005/ProgrammingLayout.txt。
APROM 0x0 使用 DNN_NuOne_UAC.bin；DataFlash 0x15000 使用 AirconDataFlash.bin。
AirconDataFlash.bin 包含國台語模型及 0x70000 的內部 NuOne 回覆音訊。
DSMT 模型修改完整儲存後，雙擊 BuildLatestModel.cmd 更新發布檔，不需 Python。

完整口說測試：DSMT/Test/UserCommandTestGuide.txt。
兩語命令／MapID／回覆：DSMT/Test/CommandMapIDQuickReference.md。
回覆定義與來源：AudioRes/ReplyPhrases/ReplyProfile.json。
回覆格式為 16 kHz、NuOne 1.0 bits/sample；兩語命令共用國語回覆。
溫度設定回覆「好的，溫度已設定」，定時設定回覆「好的，定時已設定」，數值由 UART 核對。

控制等待預設 60 秒，可修改 DNN_VR/AirconVR.h 的 AIRCON_CONTROL_TIMEOUT_SECONDS。
語音成功解碼時輸出 UART、播放回覆並讓 PB0／D4 LED 短閃。
冷氣 GPIO、通訊與實際動作由應用層串接。
