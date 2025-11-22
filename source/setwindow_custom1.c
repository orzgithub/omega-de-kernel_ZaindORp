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

#include "ff.h"

#include "images.h"

#define LINE_TOTAL 2
#define Y_OFFSET 24
#define LINE_HEIGHT 17
#define OPTION_X (240 - 20)
#define SELECTION_MARK_X (240 - 26)
#define SELECTION_MARK_WIDTH 4
#define SELECTION_MARK_HEIGHT 4
#define TEXT_X_OFFSET 1

#define FLAG_USETHEME "USETHEME"
#define FLAG_SAVBAK "SAVBAK"
#define FLAG_TRANSGENDER "I_SUPPORT_TRANSGENDER"

TCHAR flag_path_buffer[50];
FIL flag_file;

void save_set_custom_info(TCHAR* flag, u8 status) {
    sprintf(flag_path_buffer, "%s/%s", FLAGS_FOLDER, flag);
    if (status) {
        f_mkdir(FLAGS_FOLDER);
        f_open(&flag_file, flag_path_buffer, FA_WRITE | FA_CREATE_ALWAYS);
        f_close(&flag_file);
    }
    else {
        f_unlink(flag_path_buffer);
    }
}

u8 get_set_custom_info(TCHAR* flag) {
    sprintf(flag_path_buffer, "%s/%s", FLAGS_FOLDER, flag);
    return f_stat(flag_path_buffer, NULL) == FR_OK ? 1 : 0;
}

extern void Draw_select_icon(u32 X, u32 Y, u32 mode);

//---------------------------------------------------------------------------------
u32 Setting_window_custom1(void)
{
    u16 keys;
    u32 select = 0;
    char msg[128];
    u16 clean_color;
    u32 re_show = 1;

    u8 status_use_theme = get_set_custom_info(FLAG_USETHEME);
    u8 status_save_backup = get_set_custom_info(FLAG_SAVBAK);
    u8 status_transgender_flag = get_set_custom_info(FLAG_TRANSGENDER);

	#define SELECTION_MARK_Y(line) (Y_OFFSET + (line) * LINE_HEIGHT + 2 + (8 - SELECTION_MARK_HEIGHT) / 2)

    while(1)
    {
        VBlankIntrWait(); 	
        
        if(re_show)
        {
            sprintf(msg, "%s", gl_set_custom_use_theme);
            DrawHZText12(msg, 0, TEXT_X_OFFSET, Y_OFFSET + LINE_HEIGHT * 0, gl_color_selected, 1);
            Draw_select_icon(OPTION_X, Y_OFFSET + LINE_HEIGHT * 0, status_use_theme);

            sprintf(msg, "%s", gl_set_custom_save_backup);
            DrawHZText12(msg, 0, TEXT_X_OFFSET, Y_OFFSET + LINE_HEIGHT * 1, gl_color_selected, 1);
            Draw_select_icon(OPTION_X, Y_OFFSET + LINE_HEIGHT * 1, status_save_backup);

            /*
			sprintf(msg, "%s", gl_set_custom_transgender_flag);
            DrawHZText12(msg, 0, TEXT_X_OFFSET, Y_OFFSET + LINE_HEIGHT * 2, gl_color_selected, 1);
            Draw_select_icon(OPTION_X, Y_OFFSET + LINE_HEIGHT * 2, status_transgender_flag);
			*/

            for(u32 line = 0; line < LINE_TOTAL; line++)
            {
                clean_color = (line == select) ? gl_color_btn_clean : gl_color_MENU_btn;
                Clear(SELECTION_MARK_X, SELECTION_MARK_Y(line), SELECTION_MARK_WIDTH, SELECTION_MARK_HEIGHT, clean_color, 1);
            }
            VBlankIntrWait();	
            re_show = 0;
        }
            
        scanKeys();
        keys = keysDown();
        
        if (keys & KEY_A) {
            switch(select) {
                case 0:
                    status_use_theme = !status_use_theme;
                    save_set_custom_info(FLAG_USETHEME, status_use_theme);
					Themes_init();
                    break;
                case 1:
                    status_save_backup = !status_save_backup;
                    save_set_custom_info(FLAG_SAVBAK, status_save_backup);
                    break;
                case 2:
                    status_transgender_flag = !status_transgender_flag;
                    save_set_custom_info(FLAG_TRANSGENDER, status_transgender_flag);
                    break;
            }
            re_show = 1;
        }
        else if (keys & KEY_DOWN) {  
            if(select < (LINE_TOTAL - 1)){
                select++;		
                re_show = 1;
            }
        }
        else if (keys & KEY_UP) {
            if(select > 0){
                select--;
                re_show = 1;
            }
        }  		
        else if (keys & KEY_L) {
            return 0;
        }
        else if (keys & KEY_R) {
            return 1;
        }
    }
}