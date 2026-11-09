//------------------------------------------------------------------
// Instant cheat support.
//
// A separate in-game cheat stub (source/gba_instant_cheat.s) is placed
// behind the end of the ROM instead of inside the last block, so that the
// game data at the end of the ROM is never touched.  The stub gets a small
// payload (built here) that contains:
//   - the list of cheat items (name + first entry + entry count)
//   - all cheat entries (address/value byte pairs)
//   - a minimal 12x12 glyph atlas taken from the kernel fonts
//   - the name strings as glyph index lists
// While the game runs, the stub applies every enabled item on each IRQ and
// offers a menu (RESET / SAVE / LOAD / CODES) where each item can be toggled
// individually.
//
// Used whenever the patch (stub + payload) still fits in the area the boot
// code chose for it (tail padding, or the page behind the ROM image); if it
// does not fit, the game silently keeps the old whole-list cheat patch.
//------------------------------------------------------------------
#include <gba_base.h>
#include <stdio.h>
#include <string.h>

#include "ezkernel.h"
#include "showcht.h"
#include "set_flag.h"
#include "GBApatch.h"

extern FIL gfile;
extern TCHAR currentpath[];

extern const unsigned char acHZK12[];
extern const unsigned char ASC_DATA[];

unsigned char HexToChar(unsigned char bChar);
u32 Change2cht_folder(u32 chtname);

//payload layout, must match source/gba_instant_cheat.s
//header:
//  0x00 u32 'ICHT'
//  0x04 u32 item count
//  0x08 u32 entry count
//  0x0C u32 glyph count
//  0x10 u32 items offset
//  0x14 u32 entry address table offset
//  0x18 u32 entry value table offset
//  0x1C u32 glyph width bitmap offset
//  0x20 u32 glyph bitmap offset
//  0x24 u32 name blob offset
//all sections are packed right after each other, so no matter how many
//cheats a game has only the bytes that are really used get reserved.
#define IC_BASE		IC_PAYLOAD_BASE
#define IC_WORK		0x06012C00

#define PL_HDR		0x0028
#define PL_END		0x2000

#define IC_MAX_ITEMS	48
#define IC_MAX_ENTRIES	512
#define IC_MAX_GLYPHS	255	//0xFF ends a name
#define IC_MAX_NAMES	0x800

u32 gl_ic_active;
u32 gl_ic_length;

static u32 ic_items;
static u32 ic_entries;
static u32 ic_glyphs;
static u32 ic_names;	//name blob size in bytes
static u32 ic_count;	//1: count only, 0: write the payload
static u32 ic_o_items;
static u32 ic_o_addrs;
static u32 ic_o_vals;
static u32 ic_o_widths;
static u32 ic_o_glyphs;
static u32 ic_o_names;
static u8* ic_p;
static u8* ic_e;
static u8 ic_section[64];	//current [section] title, without the brackets
static u32 ic_slen;
static u32 ic_skip_section;	//inside [GameInfo]: no cheats there
static u32 ic_sel;	//current [section] was selected in the cheat window

static vu16* const ic_codes = (vu16*)IC_WORK;

//------------------------------------------------------------------
static void ic_w16(u32 off,u16 v)
{
	*(vu16*)(IC_BASE+off) = v;
}

static void ic_w32(u32 off,u32 v)
{
	*(vu32*)(IC_BASE+off) = v;
}

//VRAM only accepts 16 bit writes, so single bytes are patched into the
//halfword that contains them
static void ic_w8(u32 off,u8 v)
{
	vu16* p = (vu16*)(IC_BASE+(off&~1u));
	u16 cur = *p;
	if(off&1) cur = (u16)((cur&0x00FF) | ((u16)v<<8));
	else cur = (u16)((cur&0xFF00) | (u16)v);
	*p = cur;
}

static int ic_hex(u8 c)
{
	if(c>='0' && c<='9') return c-'0';
	if(c>='a' && c<='f') return c-'a'+10;
	if(c>='A' && c<='F') return c-'A'+10;
	return -1;
}

//------------------------------------------------------------------
//add one glyph to the atlas, returns the glyph index (or -1)
static int ic_glyph(u16 code)
{
	u32 g,r,w;
	u16 rows[12];
	const u8* src;

	for(g=0;g<ic_glyphs;g++)
	{
		if(ic_codes[g]==code) return (int)g;
	}
	if(ic_glyphs>=IC_MAX_GLYPHS) return -1;

	if(code < 0x80)
	{
		src = ASC_DATA + (u32)code*12;
		for(r=0;r<12;r++) rows[r] = (u16)(src[r]) << 8;
		w = 8;
	}
	else
	{
		u32 idx;
		u32 hi = code>>8;
		u32 lo = code&0xFF;
		if(hi < 0xB0) idx = (hi-0xA1)*94 + (lo-0xA1);
		else idx = 9*94 + (hi-0xB0)*94 + (lo-0xA1);
		if(idx >= 7614) return -1;
		src = acHZK12 + idx*24;
		//the kernel font stores the left 8 columns in byte 0 and the next 8
		//columns in byte 1 (bits 7..4 hold columns 8..11)
		for(r=0;r<12;r++) rows[r] = (u16)(((u16)src[r*2]<<8) | (u16)src[r*2+1]);
		w = 12;
	}

	if(ic_count==0)
	{
		u8* dst = (u8*)(IC_BASE+ic_o_glyphs+ic_glyphs*24);
		for(r=0;r<12;r++) *(vu16*)(dst+r*2) = rows[r];
		if(w==12)
		{
			u32 bo = ic_o_widths+(ic_glyphs>>3);
			u8 bit = (u8)(1<<(ic_glyphs&7));
			u8 cur = 0;
			//RMW through the 16 bit helper
			{
				vu16* p = (vu16*)(IC_BASE+(bo&~1u));
				u16 hw = *p;
				cur = (bo&1) ? (u8)(hw>>8) : (u8)hw;
				cur |= bit;
			}
			ic_w8(bo,cur);
		}
	}
	ic_codes[ic_glyphs] = code;
	return (int)ic_glyphs++;
}

//------------------------------------------------------------------
//append a name (GBK or ASCII) to the name blob, returns new byte count
static u32 ic_name(u8* k,u8* ke,u32 n)
{
	while(k<ke)
	{
		u16 code;
		int g;
		if(*k < 0x80){ code = *k; k++; }
		else if((k+1)<ke){ code = (u16)(((u16)(*k)<<8) | k[1]); k+=2; }
		else break;
		if(code==0) break;
		g = ic_glyph(code);
		if(g<0) continue;
		if(n>=IC_MAX_NAMES) break;
		if(ic_count==0) ic_w8(ic_o_names+n,(u8)g);
		n++;
	}
	return n;
}

//------------------------------------------------------------------
//append the [section] title in front of the item name, shortened so that the
//actual cheat name still fits on the screen
static u32 ic_name_section(u32 n)
{
	u32 cells = 0;
	u8* k = ic_section;
	u8* ke = ic_section+ic_slen;
	while(k<ke && cells<14)
	{
		u16 code;
		int g;
		if(*k < 0x80){ code = *k; k++; }
		else if((k+1)<ke){ code = (u16)(((u16)(*k)<<8) | k[1]); k+=2; }
		else break;
		if(code==' ') continue;
		g = ic_glyph(code);
		if(g<0) continue;
		if(n>=IC_MAX_NAMES) break;
		if(ic_count==0) ic_w8(ic_o_names+n,(u8)g);
		n++;
		cells += (code<0x80)?1:2;
	}
	if(n>0)
	{
		int g = ic_glyph((u16)' ');
		if(g>=0 && n<IC_MAX_NAMES)
		{
			if(ic_count==0) ic_w8(ic_o_names+n,(u8)g);
			n++;
		}
	}
	return n;
}

//------------------------------------------------------------------
//parse the value part of one cheat line: "ADDR v1,v2;ADDR2 v3;"
static void ic_add_item(u8* ks,u8* ke,u8* vs,u8* ve)
{
	u32 first_entry = ic_entries;
	u32 addr = 0;
	u32 n0 = ic_names;
	u32 n;
	u32 n1;
	u8* p = vs;
	int expect_addr = 1;

	while(ks<ke && (*ks==' ' || *ks=='\t')) ks++;
	while(ke>ks && (ke[-1]==' ' || ke[-1]=='\t' || ke[-1]=='\r')) ke--;
	if(ke<=ks) return;

	while(p<ve)
	{
		int h = ic_hex(*p);
		if(h>=0)
		{
			u32 v = 0;
			while(p<ve && (h=ic_hex(*p))>=0)
			{
				v = (v<<4) | (u32)h;
				p++;
			}
			if(expect_addr)
			{
				addr = v;
				expect_addr = 0;
			}
			else if(ic_entries<IC_MAX_ENTRIES)
			{
				//.cht addresses are relative to the EWRAM/IWRAM window: rebase
				//them exactly like the old cheat patch does
				u32 t = (addr>=0x40000) ? ((addr&0x7FFF)+0x03000000)
				                        : ((addr&0x3FFFF)+0x02000000);
				if(ic_count==0)
				{
					*(vu32*)(IC_BASE+ic_o_addrs+ic_entries*4) = t;
					ic_w8(ic_o_vals+ic_entries,(u8)(v&0xFF));
				}
				ic_entries++;
				addr++;
			}
			continue;
		}
		if(*p==';')
		{
			expect_addr = 1;
			addr = 0;
		}
		p++;
	}

	if(ic_entries==first_entry) return;		//no code at all
	if(ic_items>=IC_MAX_ITEMS){ ic_entries = first_entry; return; }

	n = ic_name_section(n0);
	n1 = ic_name(ks,ke,n);
	if(n==n1)
	{
		//no drawable character at all: name the entry by its index instead
		u32 v = ic_items+1;
		int g;
		if(v>99) v = 99;
		if(v>=10)
		{
			g = ic_glyph((u16)('0'+(v/10)));
			if(g<0){ ic_entries = first_entry; return; }
			if(ic_count==0) ic_w8(ic_o_names+n,(u8)g);
			n++;
		}
		g = ic_glyph((u16)('0'+(v%10)));
		if(g<0){ ic_entries = first_entry; return; }
		if(ic_count==0) ic_w8(ic_o_names+n,(u8)g);
		n++;
	}
	else
	{
		n = n1;
	}
	if(n>=IC_MAX_NAMES){ ic_entries = first_entry; return; }
	if(ic_count==0) ic_w8(ic_o_names+n,0xFF);

	{
		u32 off = ic_o_items + ic_items*8;
		if(ic_count==0)
		{
			ic_w16(off+0,(u16)n0);
			ic_w16(off+2,(u16)first_entry);
			ic_w16(off+4,(u16)(ic_entries-first_entry));
			ic_w16(off+6,0);
		}
	}
	ic_names = n+1;
	ic_items++;
}

//------------------------------------------------------------------

//find the end of a value string (handles wrapped lines)
static u8* ic_value_end(u8* p)
{
	u8* q = p;
	while(q<ic_e)
	{
		if(*q=='\n')
		{
			u8* r = q+1;
			u8* t;
			while(r<ic_e && (*r==' ' || *r=='\t' || *r=='\r')) r++;
			if(r>=ic_e) return q;
			if(*r=='[') return q;
			if(*r=='-' && (r+1)<ic_e && r[1]=='-') return q;
			if(*r=='#') return q;
			if(*r=='/' && (r+1)<ic_e && (r[1]=='/' || r[1]=='*')) return q;
			t = r;
			while(t<ic_e && *t!='\n' && *t!='=' && *t!=';') t++;
			if(t<ic_e && *t=='=') return q;
		}
		q++;
	}
	return ic_e;
}

//------------------------------------------------------------------
static void ic_build_once(u8* p,u8* e)
{
	ic_p = p;
	ic_e = e;
	ic_items = 0;
	ic_entries = 0;
	ic_glyphs = 0;
	ic_names = 0;
	ic_slen = 0;
	ic_skip_section = 0;
	ic_sel = 0;

	while(ic_p<ic_e)
	{
		u8* ks;
		u8* keq;
		u8* kend;
		u8* ve;

		while(ic_p<ic_e && (*ic_p==' ' || *ic_p=='\t' || *ic_p=='\r' || *ic_p=='\n')) ic_p++;
		if(ic_p>=ic_e) break;

		if(*ic_p=='-')
		{
			u8* q = ic_p;
			while(q<ic_e && *q=='-') q++;
			if(q>ic_p && (q>=ic_e || *q=='\n' || *q=='\r'))
			{
				//a '----' ruler line is a separator, not the end of the file
				ic_p = q;
				continue;
			}
		}
		if(*ic_p=='#' || (*ic_p=='/' && (ic_p+1)<ic_e && (ic_p[1]=='/' || ic_p[1]=='*')))
		{
			while(ic_p<ic_e && *ic_p!='\n') ic_p++;
			continue;
		}
		if(*ic_p=='[')
		{
			u8* q = ic_p+1;
			ic_slen = 0;
			while(q<ic_e && *q!='\n' && *q!=']')
			{
				if(ic_slen < sizeof(ic_section)) ic_section[ic_slen++] = *q;
				q++;
			}
			ic_skip_section = (ic_slen==8 && memcmp(ic_section,"GameInfo",8)==0);
			ic_sel = Cheat_SectionSelected(ic_section,ic_slen);
			while(ic_p<ic_e && *ic_p!='\n') ic_p++;
			continue;
		}
		if(ic_skip_section || ic_sel==0)
		{
			while(ic_p<ic_e && *ic_p!='\n') ic_p++;
			continue;
		}

		ks = ic_p;
		keq = 0;
		while(ic_p<ic_e && *ic_p!='\n')
		{
			if(*ic_p=='='){ keq = ic_p; break; }
			ic_p++;
		}
		if(keq==0)
		{
			while(ic_p<ic_e && *ic_p!='\n') ic_p++;
			continue;
		}

		kend = keq;
		ic_p = keq+1;
		ve = ic_value_end(ic_p);
		if(ic_items>=IC_MAX_ITEMS || ic_entries>=IC_MAX_ENTRIES) break;
		ic_add_item(ks,kend,ic_p,ve);
		ic_p = ve;
	}
}

static int ic_build(u8* p,u8* e)
{
	u32 total;

	//pass 1: only count items/entries/glyphs/name bytes
	ic_count = 1;
	ic_build_once(p,e);
	if(ic_items>=IC_MAX_ITEMS) return 0;

	//pack the sections right after each other
	ic_o_items  = PL_HDR;
	ic_o_addrs  = (ic_o_items + ic_items*8 + 3) & ~3u;
	ic_o_vals   = ic_o_addrs + ic_entries*4;
	ic_o_widths = (ic_o_vals + ic_entries + 1) & ~1u;
	ic_o_glyphs = (ic_o_widths + ((ic_glyphs+7)>>3) + 3) & ~3u;
	ic_o_names  = ic_o_glyphs + ic_glyphs*24;
	total = ic_o_names + ic_names;
	if(total > PL_END) return 0;

	//pass 2: write it
	ic_count = 0;
	{
		u32 n = ((ic_glyphs+7)>>3);
		u32 i;
		for(i=0;i<n;i+=2) *(vu16*)(IC_BASE+ic_o_widths+i) = 0;
	}
	ic_build_once(p,e);
	if(ic_items>=IC_MAX_ITEMS) return 0;

	ic_w32(0x00,0x54484349);	//'ICHT'
	ic_w32(0x04,ic_items);
	ic_w32(0x08,ic_entries);
	ic_w32(0x0C,ic_glyphs);
	ic_w32(0x10,ic_o_items);
	ic_w32(0x14,ic_o_addrs);
	ic_w32(0x18,ic_o_vals);
	ic_w32(0x1C,ic_o_widths);
	ic_w32(0x20,ic_o_glyphs);
	ic_w32(0x24,ic_o_names);
	gl_ic_length = (ic_o_names + ic_names + 3) & ~3;
	return 1;
}

//------------------------------------------------------------------
//keep only the file name (drop any folder prefix)
static TCHAR* ic_basename(TCHAR* p)
{
	TCHAR* b = p;
	while(*p)
	{
		if(*p=='/' || *p=='\\') b = p+1;
		p++;
	}
	return b;
}

//locate and open the .cht file for a game, leaves it open
static int ic_open_cht(TCHAR* gamefilename)
{
	u32 res;
	UINT ret;
	TCHAR name[100];
	TCHAR* base;
	u32 GAMEID = 0;
	u32 len;

	res = f_open(&gfile,gamefilename,FA_READ);
	if(res==FR_OK)
	{
		f_lseek(&gfile,0xAC);
		f_read(&gfile,&GAMEID,4,&ret);
		f_close(&gfile);
	}
	if(GAMEID==0) memcpy(&GAMEID,GAMECODE,4);	//fall back to the gamecode read at boot

	base = ic_basename(gamefilename);
	memset(name,0,100);
	memcpy(name,base,99);
	len = strlen(name);
	if(len<4) return 0;
	name[len-3] = 'c';
	name[len-2] = 'h';
	name[len-1] = 't';

	//1) the folder the rom lives in
	if(f_open(&gfile,name,FA_READ)==FR_OK) return 1;

	//2) the cheat folder
	if(f_chdir(CHEAT_FOLDER)!=FR_OK) return 0;
	if(f_open(&gfile,name,FA_READ)==FR_OK) return 1;
	if(GAMEID==0) return 0;

	//3) the GameID -> cht name table
	res = f_open(&gfile,"GameID2cht.bin",FA_READ);
	if(res==FR_OK)
	{
		u32 filesize = f_size(&gfile);
		u32* tempbuff = (u32*)pReadCache;
		u32 i;
		if(filesize>0x10000) filesize = 0x10000;
		f_lseek(&gfile,0);
		f_read(&gfile,tempbuff,filesize,&ret);
		for(i=0;i<filesize/4;i+=2)
		{
			if(tempbuff[i]==GAMEID)
			{
				u32 chtname = ((u32*)tempbuff)[i+1];
				f_close(&gfile);
				if(Change2cht_folder(chtname)!=FR_OK) return 0;
				memset(name,0,100);
				sprintf(name,"%d%d%d%d.cht",
					HexToChar(((u8*)&chtname)[0]),HexToChar(((u8*)&chtname)[1]),
					HexToChar(((u8*)&chtname)[2]),HexToChar(((u8*)&chtname)[3]));
				if(f_open(&gfile,name,FA_READ)==FR_OK) return 1;
				return 0;
			}
		}
		f_close(&gfile);
	}
	return 0;
}

//------------------------------------------------------------------
//called before the ROM is loaded/patched, returns 1 when the instant
//cheat stub can be used
//bytes needed on the rom for the instant-cheat stub plus its payload
//bytes of the instant-cheat stub that are actually written: the RTS preview
//renderer lives behind IC_no_preview_end and is only appended when the
///FLAGS/RTS_PREVIEW option is on.  Everything that needs the stub size (the
//writer, the .pat cache and the VRAM budget) has to use this one rule.
u32 InstantCheat_StubSize(void)
{
	return gl_rts_preview_on
		? ((u32)IC_ReplaceIRQ_end - (u32)IC_ReplaceIRQ_start)
		: ((u32)IC_no_preview_end - (u32)IC_ReplaceIRQ_start);
}

u32 InstantCheat_PatchSize(void)
{
	return InstantCheat_StubSize() + gl_ic_length;
}

u32 InstantCheat_Prepare(TCHAR* gamefilename,u32 filesize,u32 savemode)
{
	u32 len;
	UINT ret;

	gl_ic_active = 0;
	gl_ic_length = 0;
	ic_items = 0;
	ic_glyphs = 0;
	ic_names = 0;
	(void)savemode;	//space is checked by the caller (tail or block behind the rom)

	f_chdir(currentpath);

	if(!get_set_custom_info(FLAG_ENHANCED_CHEAT)) goto done;	//custom setting
	if(filesize==0) goto done;
	//Diablo keeps live code in the last kilobytes of the image, it needs the
	//dedicated DLAB layout, so it keeps the old cheat patch
	if(*(u32*)GAMECODE==0x42414C44) goto done;

	if(ic_open_cht(gamefilename)==0) goto done;

	len = f_size(&gfile);
	if(len>0x1E000) len = 0x1E000;
	f_lseek(&gfile,0);
	if(f_read(&gfile,pReadCache,len,&ret)!=FR_OK)
	{
		f_close(&gfile);
		goto done;
	}
	f_close(&gfile);

	if(ic_build(pReadCache,pReadCache+ret)==0) goto done;
	gl_ic_active = 1;

done:
	f_chdir(currentpath);
	return gl_ic_active;
}
