#include <gba_systemcalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <gba_base.h>
#include <gba_input.h>

#include "ez_define.h"
#include "lang.h"
#include "ezkernel.h"
#include "RTC.h"
#include "draw.h"
#include "Ezcard_OP.h"

#include "images.h"
#include "transflag.h"

#define USEEASTER 0
// These easters were broken on newer version of Devkit pro so set to 1 only when you are using an older compliator.

#if USEEASTER == 1
	#include "gamemine.h"
#endif

FIL filecheck;

extern u16 gl_select_lang;
u16 Rev;
//---------------------------------------------------------------------------------
void Show_ver(void)
{
	char msg[20];
	char *ver="K:1.04";
	u16 FPGAver = Read_FPGA_ver();
	sprintf(msg,"FW:%d %s",FPGAver&0xFF,ver);
	DrawHZText12(msg,0,160,3, gl_color_text,1);	
	Rev = FPGAver & 0xF000;
}
//---------------------------------------------------------------------------------
u32 Show_help_window()
{
	char msg[20];
	u16 tapa = 0;
	Show_ver();
	if(gl_select_lang == 0xE1E1)//english
	{
		DrawPic((u16*)gImage_English_manual, 240-70, 160-70, 70, 70, 0, 0, 1);//
	}
	else{
		DrawPic((u16*)gImage_Chinese_manual, 240-70, 160-70, 70, 70, 0, 0, 1);//
	}
	if (f_open(&filecheck,"/FLAGS/I_SUPPORT_TRANSGENDER", FA_OPEN_EXISTING) != FR_OK){
		DrawPic((u16*)transflag, 240-70-14, 160-70, 14, 70, 0, 0, 1);
	}
	DrawHZText12("START  :",0,3,20, gl_color_selected,1);
		DrawHZText12(gl_START_help,0,52,20, gl_color_text,1);
		
	DrawHZText12("SELECT :",0,3,35, gl_color_selected,1);
		DrawHZText12(gl_SELECT_help,0,52,35, gl_color_text,1);
		
	DrawHZText12("L + A  :",0,3,50, gl_color_selected,1);
		DrawHZText12(gl_L_A_help,0,52,50, gl_color_text,1);
		
	DrawHZText12("L+START:",0,3,65, gl_color_selected,1);
		DrawHZText12(gl_LSTART_help,0,52,65, gl_color_text,1);	
	
	DrawHZText12(gl_modver,0,3,134 ,gl_color_text,1);
	
	sprintf(msg,"Rev.%s",((Rev==0xB000)||(Rev==0xA000)) ? "B":"A");
	DrawHZText12(msg,0,3,149, gl_color_text,1);	
				
	DrawHZText12(gl_online_manual,0,240-70-7,77, gl_color_text,1);
	while(1)
	{
		#if USEEASTER == 1
			if (tapa == 4){
				gameMine();
			}
		#endif
		VBlankIntrWait(); 	
		scanKeys();
		u16 keys = keysDown();
		if (keys & KEY_L) {//return
			return 0;
		}		
		else if (keys & KEY_R){
			return 1;
		}
		#if USEEASTER == 1
			else if (keys & KEY_A){
				tapa++;
			}
		#endif
	}
}