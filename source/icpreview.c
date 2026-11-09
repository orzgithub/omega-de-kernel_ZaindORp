/* Instant-cheat in-game menu: render a small preview of the saved frame.
 * Runs from ROM, called from the ARM stub (source/gba_instant_cheat.s).
 * Only touches CPSR-visible CPU + CPLD registers; no kernel globals.
 * Saved data layout (see call_Save in gba_instant_cheat.s):
 *   page 0x80 : IWRAM image (0x8000), palette at +0x8000 (0x400)
 *   page 0x90 : VRAM 0x06000000..0x0600FFFF (0x10000 bytes)
 *   page 0xA0 : VRAM 0x06010000..0x06017FFF (0x8000 bytes)
 *               OAM  at +0x8000 (0x400), IO at +0x9000 (0x400 area)
 */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef short s16;
typedef int s32;
typedef volatile u16 vu16;

#define REG16(a) (*(vu16*)(a))
#define CPLD_PAGE 0x09C00000
#define SRAM_WIN  0x0E000000
#define SCREEN    0x06000000

#define PREV_X 120
#define PREV_Y 40
#define PREV_W 120
#define PREV_H 79

#define IC_ABS(x) ((x) < 0 ? -(x) : (x))

static inline __attribute__((always_inline)) void ic_setpage(u32 page)
{
	REG16(0x09FE0000) = 0xD200;
	REG16(0x08000000) = 0x1500;
	REG16(0x08020000) = 0xD200;
	REG16(0x08040000) = 0x1500;
	REG16(CPLD_PAGE)  = (u16)page;
	REG16(0x09FC0000) = 0x1500;
}

/* two VRAM windows while rendering: 0x90 covers [0,0x10000), 0xA0 covers [0x10000,0x18000) */
static u32 cur_page;
static inline __attribute__((always_inline)) void ic_vram_page(u32 page)
{
	if (page != cur_page) { ic_setpage(page); cur_page = page; }
}

static inline __attribute__((always_inline)) u16 ic_vram16(u32 off)
{
	if (off < 0x10000) { ic_vram_page(0x90); return REG16(SRAM_WIN + (off & 0xFFFE)); }
	ic_vram_page(0xA0); return REG16(SRAM_WIN + ((off - 0x10000) & 0x7FFE));
}
static inline __attribute__((always_inline)) u8 ic_vram8(u32 off)
{
	if (off < 0x10000) { ic_vram_page(0x90); return *(volatile u8*)(SRAM_WIN + off); }
	ic_vram_page(0xA0); return *(volatile u8*)(SRAM_WIN + (off - 0x10000));
}

/* scratch lives in VRAM past the visible framebuffer; backup/restore_LCD
   covers the whole 0x18000 bytes of VRAM, so it is restored on menu exit. */
#define IC_SCRATCH 0x06013000
#define ic_pal  ((u16*)(IC_SCRATCH + 0x000))
#define ic_oam  ((u16*)(IC_SCRATCH + 0x400))
#define ic_io   ((u16*)(IC_SCRATCH + 0xC00))

static inline __attribute__((always_inline)) void ic_load_saved(void)
{
	u32 i;
	ic_setpage(0x80);
	for (i = 0; i < 256; i++) ic_pal[i] = REG16(SRAM_WIN + 0x8000 + i * 2);
	ic_setpage(0xA0);
	for (i = 0; i < 512; i++) ic_oam[i] = REG16(SRAM_WIN + 0x8000 + i * 2);
	for (i = 0; i < 0x50; i++) ic_io[i] = REG16(SRAM_WIN + 0x9000 + i * 2);
}

#define IO(a) (ic_io[((a) - 0x04000000) >> 1])

static inline __attribute__((always_inline)) u16 ic_bgcnt(u32 n) { return IO(0x04000008 + n * 2); }
static inline __attribute__((always_inline)) u16 ic_bghofs(u32 n) { return IO(0x04000010 + n * 4); }
static inline __attribute__((always_inline)) u16 ic_bgvofs(u32 n) { return IO(0x04000012 + n * 4); }

/* ---- text BG (modes 0/1) ---- */
static inline __attribute__((always_inline)) int ic_text_bg(u32 n, int x, int y, u16 *out, int *prio)
{
	u16 cnt = ic_bgcnt(n);
	u32 w, h, sb, cb, px, py, mapx, mapy, tile, addr;
	int hofs = ic_bghofs(n) & 0x1FF, vofs = ic_bgvofs(n) & 0x1FF;
	int bw, bh;
	int flipx, flipy, pal, idx, bpp8;
	switch ((cnt >> 14) & 3) {
	case 0: bw = 256; bh = 256; break;
	case 1: bw = 512; bh = 256; break;
	case 2: bw = 256; bh = 512; break;
	default: bw = 512; bh = 512; break;
	}
	w = bw; h = bh;
	bpp8 = (cnt & 0x80) != 0;
	cb = ((cnt >> 2) & 3) * 0x4000;
	sb = ((cnt >> 8) & 0x1F) * 0x800;
	px = ((u32)(x + hofs)) & (w - 1);
	py = ((u32)(y + vofs)) & (h - 1);
	mapx = px & 255; mapy = py & 255;
	sb += ((py >> 8) * (w >> 8) + (px >> 8)) * 0x800;
	tile = ic_vram16(sb + (mapy >> 3) * 64 + ((mapx >> 3) << 1));
	flipx = (tile & 0x400) != 0;
	flipy = (tile & 0x800) != 0;
	pal = (tile >> 12) & 0xF;
	if (flipx) mapx = 7 - (mapx & 7); else mapx &= 7;
	if (flipy) mapy = 7 - (mapy & 7); else mapy &= 7;
	tile &= 0x3FF;
	if (bpp8) {
		addr = cb + (tile << 6) + (mapy << 3) + mapx;
		idx = ic_vram8(addr);
		if (idx == 0) return 0;
	} else {
		addr = cb + (tile << 5) + (mapy << 2) + (mapx >> 1);
		idx = ic_vram8(addr);
		idx = (mapx & 1) ? (idx >> 4) : (idx & 0xF);
		if (idx == 0) return 0;
		idx += pal * 16;
	}
	*out = ic_pal[idx & 0xFF];
	*prio = cnt & 3;
	return 1;
}

/* ---- affine BG (mode 1 BG2 / mode 2 BG2,BG3) ---- */
static inline __attribute__((always_inline)) int ic_aff_bg(u32 n, int x, int y, u16 *out, int *prio)
{
	u16 cnt = ic_bgcnt(n);
	s32 pa = (s16)IO(0x04000020 + n * 0x10);
	s32 pb = (s16)IO(0x04000022 + n * 0x10);
	s32 pc = (s16)IO(0x04000024 + n * 0x10);
	s32 pd = (s16)IO(0x04000026 + n * 0x10);
	s32 refx = (s16)IO(0x04000028 + n * 0x10);
	s32 refy = (s16)IO(0x0400002C + n * 0x10);
	u32 w = 128u << ((cnt >> 14) & 3);
	u32 cb = ((cnt >> 2) & 3) * 0x4000;
	u32 tx, ty, mapx, mapy, sx, sy, t;
	tx = (u32)(((pa * (x << 8)) + (pb * (y << 8)) + (refx << 8)) >> 8);
	ty = (u32)(((pc * (x << 8)) + (pd * (y << 8)) + (refy << 8)) >> 8);
	mapx = tx & (w - 1);
	mapy = ty & (w - 1);
	sx = mapx >> 3;
	sy = mapy >> 3;
	{
		u32 n2 = w >> 8;
		u32 tile = ((sy * n2 + sx) & 0x3FF);
		t = ic_vram8(cb + (tile << 6) + ((mapy & 7) << 3) + (mapx & 7));
	}
	if (t == 0) return 0;
	*out = ic_pal[t];
	*prio = cnt & 3;
	return 1;
}

/* ---- sprites (regular, non-affine) ---- */

static inline __attribute__((always_inline)) int ic_obj(int x, int y, u16 *out, int *prio)
{
	int i;
	for (i = 127; i >= 0; i--) {
		u16 a0 = ic_oam[i * 4 + 0], a1 = ic_oam[i * 4 + 1], a2 = ic_oam[i * 4 + 2];
		int shape, sz, w, h, sx, sy, px, py, bpp8, hflip, vflip, pal, idx;
		u32 addr;
		if ((a0 & 0x300) == 0x200) continue;
		if (a0 & 0x100) continue; /* affine: unsupported in preview */
		shape = (a0 >> 14) & 3;
		sz = (a1 >> 14) & 3;
		if (shape == 0) { w = 8 << sz; h = 8 << sz; }
		else if (shape == 1) { w = 8 << sz; h = (8 << sz) >> 1; }
		else if (shape == 2) { w = (8 << sz) >> 1; h = 8 << sz; }
		else continue;
		sx = (a1 & 0x1FF); if (sx > 255) sx -= 512;
		sy = (a0 & 0xFF); if (sy > 159) sy -= 256;
		px = x - sx; py = y - sy;
		if (px < 0 || py < 0 || px >= w || py >= h) continue;
		hflip = (a1 & 0x1000) != 0;
		vflip = (a1 & 0x2000) != 0;
		if (hflip) px = w - 1 - px;
		if (vflip) py = h - 1 - py;
		bpp8 = (a0 & 0x2000) != 0;
		pal = (a2 >> 12) & 0xF;
		if (a0 & 0x800) { /* 1D (tile) mapping */
			u32 tile = a2 & 0x3FF;
			if (bpp8) {
				addr = (tile << 5) + ((py >> 3) * (w >> 3) + (px >> 3)) * 64 + ((py & 7) << 3) + (px & 7);
			} else {
				addr = (tile << 5) + ((py >> 3) * (w >> 3) + (px >> 3)) * 32 + ((py & 7) << 2) + ((px & 7) >> 1);
			}
		} else {
			u32 row = (py & 7) >> 3;
			(void)row;
			if (bpp8) addr = ((a2 & 0x3FF) << 5) + ((py >> 3) * 0x80) + ((py & 7) << 3) + (px & 7) + ((a2 & 0x3FF) * 0);
			else addr = ((a2 & 0x3FF) << 5) + ((py >> 3) * 0x40) + ((py & 7) << 2) + ((px & 7) >> 1);
			addr += 0;
		}
		addr += 0x10000;
		{
			u8 v = ic_vram8(addr);
			if (bpp8) { idx = v; if (idx == 0) continue; }
			else { idx = (px & 1) ? (v >> 4) : (v & 0xF); if (idx == 0) continue; idx += pal * 16; }
		}
		*out = ic_pal[idx & 0xFF];
		*prio = (a2 >> 10) & 3;
		return 1;
	}
	return 0;
}

__attribute__((target("arm"),noinline,section(".icprev"))) void IC_PreviewRender(void)
{
	u16 *dst = (u16*)SCREEN;
	u16 disp;
	int ty, tx;
	u32 mode;
	ic_load_saved();
	disp = IO(0x04000000);
	if (disp & 0x80) { /* forced blank */
		for (ty = 0; ty < PREV_H; ty++)
			for (tx = 0; tx < PREV_W; tx++)
				dst[(PREV_Y + ty) * 240 + PREV_X + tx] = 0;
		ic_setpage(0);
		return;
	}
	mode = disp & 7;
	for (ty = 0; ty < PREV_H; ty++) {
		int y = ty * 2;
		for (tx = 0; tx < PREV_W; tx++) {
			int x = tx * 2;
			u16 col = ic_pal[0];
			int best = 4;
			u16 c;
			int p;
			if (mode == 3) {
				u16 v = ic_vram16((u32)(y * 240 + x) * 2);
				col = v; best = 0;
			} else if (mode == 4) {
				u32 base = (disp & 0x10) ? 0xA000 : 0;
				u8 idx = ic_vram8(base + y * 240 + x);
				col = ic_pal[idx]; best = 0;
			} else if (mode == 5) {
				u32 base = (disp & 0x10) ? 0xA000 : 0;
				u16 v = ic_vram16(base + (u32)2 * (y * 160 + x));
				col = v; best = 0;
			} else {
				int pass;
				for (pass = 0; pass < 5; pass++) {
					int got = 0;
					switch (pass) {
					case 0: if (disp & 0x0100) got = ic_text_bg(0, x, y, &c, &p); break;
					case 1: if (disp & 0x0200) got = ic_text_bg(1, x, y, &c, &p); break;
					case 2: if (disp & 0x0400) {
							if (mode == 0) got = ic_text_bg(2, x, y, &c, &p);
							else got = ic_aff_bg(2, x, y, &c, &p);
						} break;
					case 3: if (disp & 0x0800) {
							if (mode == 0) got = ic_text_bg(3, x, y, &c, &p);
							else got = ic_aff_bg(3, x, y, &c, &p);
						} break;
					default: got = ic_obj(x, y, &c, &p); break;
					}
					if (got && p < best) { best = p; col = c; }
				}
				if (disp & 0x1000) { /* OBJ window unsupported: skip */ }
			}
			dst[(PREV_Y + ty) * 240 + PREV_X + tx] = col;
		}
	}
	ic_setpage(0);
}

__attribute__((target("arm"),noinline,section(".icprev"))) void IC_PreviewRenderEnd(void)
{
	/* end marker: the blob is copied verbatim into the patch area */
}
