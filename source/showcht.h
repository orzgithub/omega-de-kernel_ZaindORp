#include <gba_base.h>


#define MAX_BUF_LEN 6000
#define MAX_KEY_LEN 50
#define MAX_VAL_LEN 6000

#define MAX_sectionVAL_LEN 300

typedef struct CHT_LINE{
	char LINEname[MAX_KEY_LEN];
	//char KEY_val[256];
	u8 is_section ;
	u8 section_val_count ;
	u8 len;
	u8 select ;
} FM_CHT_LINE;

typedef struct ST_entry_{	
	u32  address;		
	u32  VAL;	
} ST_entry;

//int Get_KEY_val(FIL* file,char*KEY_section,char*KEY_secval,char getbuff[]);
int Show_all_KEY_val(FIL* file);
u32 Check_cht_file(TCHAR *gamefilename);
void Open_cht_file(TCHAR *gamefilename,u32 havecht);
void Trim(char s[]);

//sections that got at least one selected line in the cheat window
extern u32 gl_cheat_sel_count;
extern u32 gl_cheat_sel_hash[32];
u32 Cheat_SectionHash(const u8* s,u32 len);
u32 Cheat_SectionSelected(const u8* s,u32 len);