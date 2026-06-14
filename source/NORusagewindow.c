#include <gba_systemcalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <gba_base.h>
#include <gba_input.h>
#include <string.h>

#include "ez_define.h"
#include "lang.h"
#include "ezkernel.h"
#include "RTC.h"
#include "draw.h"
#include "Ezcard_OP.h"
#include "NORflash_OP.h"

#include "images.h"

extern void Draw_select_icon(u32 X, u32 Y, u32 mode);

u32 Show_NOR_usage_window() {
    u16 keys;
    DrawPic((u16*)gImage_RECENTLY, 0, 0, 240, 160, 0, 0, 1);
	DrawHZText12(gl_nor_usage,0,(240-strlen(gl_nor_usage)*6)/2,4, gl_color_text,1);

    u8 bitmap[(S29_TOTAL_BLOCKS + 7) / 8];
    GenerateNORBitmap(bitmap);

    for (u32 col = 0; col < 32; col++) {
        for (u32 row = 0; row < (S29_TOTAL_BLOCKS + 31) / 32; row++) {
            u32 block_index = row * 32 + col;
            if (block_index < S29_TOTAL_BLOCKS) {
                u8 used = (bitmap[block_index / 8] >> (block_index % 8)) & 1;
                Draw_select_icon(7 + col * 7, 20 + row * 7, !used);
            }
        }
    }

    while (1)
    {
        scanKeys();
        keys = keysDown();
        if (keys & KEY_B) {
            break;
        }
        VBlankIntrWait();
    }
    return 0;
}

u32 Show_NOR_detail_window(u8 index) {
    u16 keys;
    DrawPic((u16*)gImage_RECENTLY, 0, 0, 240, 160, 0, 0, 1);
    DrawHZText12(gl_nor_usage,0,(240-strlen(gl_nor_usage)*6)/2,4, gl_color_text,1);

    u8 bitmap[(S29_TOTAL_BLOCKS + 7) / 8];
    GenerateNORBitmap(bitmap);

    for (u32 col = 0; col < 32; col++) {
        for (u32 row = 0; row < (S29_TOTAL_BLOCKS + 31) / 32; row++) {
            u32 block_index = row * 32 + col;
            if (block_index < S29_TOTAL_BLOCKS) {
                u8 used = (bitmap[block_index / 8] >> (block_index % 8)) & 1;
                Draw_select_icon(7 + col * 7, 20 + row * 7, !used);
            }
        }
    }

    u32 start_block = pNorFS[index].rompage;
    u32 used_blocks = (pNorFS[index].filesize + S29_BLOCK_SIZE - 1) / S29_BLOCK_SIZE;
    u32 end_block = start_block + used_blocks - 1;
    
    u8 blink_status = 0;
    u8 blink_counter = 0;
    
    u32 affected_count = used_blocks;
    u32 affected_blocks[affected_count];
    for (u32 i = 0; i < used_blocks; i++) {
        u32 block = start_block + i;
        if (block < S29_TOTAL_BLOCKS) {
            affected_blocks[i] = block;
        }
    }

    while (1)
    {
        scanKeys();
        keys = keysDown();
        if (keys & KEY_B) {
            break;
        }

        VBlankIntrWait();
        
        blink_counter++;
        if (blink_counter >= 20) {
            blink_counter = 0;
            blink_status = !blink_status;
            for (u32 i = 0; i < affected_count; i++) {
                u32 block = affected_blocks[i];
                u32 col = block % 32;
                u32 row = block / 32;
                Draw_select_icon(7 + col * 7, 20 + row * 7, blink_status);
            }
        }
    }
    return 0;
}