;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


    AREA _CMDData, DATA, READONLY

    EXPORT  u32CMDDataBegin
    EXPORT  u32CMDDataEnd

u32CMDDataBegin
	INCBIN ..\CMDSet\Cinderella_pack.bin
	;INCBIN ..\CMDSet\cmd.bin
	;INCBIN ..\CMDSet\20250213_Cyberon_10Idioms_pack.bin
	
u32CMDDataEnd

	END
		
		
		
		
		
		
		
		
		