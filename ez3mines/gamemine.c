/******************************************************************
 * Mines for the GameBoy Advance
 * 
 * By Ivan Mackintosh (ivan@rcp.co.uk)
 * Art by Roger Bacon (http://www.btinternet.com/~rog666/)
 *
 * This program requires GCC to be compiled. Download it from
 * http://www.devrs.com/gba
 *
 * Version 1.0 - 20 April 2001
 *
 ******************************************************************
 */

#include "gba.h"
#include "tiles.h"
#include "title.h"

#define TILECURSOR      (1)
#define TILECHAR        (2)
#define BLANKCHAR       (3)
#define FLAGCHAR        (12)
#define MINECHAR        (13)
#define NUMBERCHARS     (14)
#define STATUSFLAG      (24)
#define BOOMTILES       (25)

enum
{
   Joystick_None = 0,
   Joystick_Up,
   Joystick_Down,
   Joystick_Left,
   Joystick_Right
};



int OldXPos, OldYPos;
int FlagCounter, TileCount;

u16 * tpal;
u8 * tiles;
u16 * map0;
u16 * map1;

#define MAX_X   (15)
#define MAX_Y   (9)
#define MINEFIELD_OFFSET        (1)

u8 MineField[MAX_X][MAX_Y];

#define MAX_MINES (20)

// Waits for n number of screen refreshes.
void Sleep(int NumSyncs)
{
   volatile u8 VCount;

   do
   {
      // wait until the start of the new redraw
      VCount = REG_VCOUNT;
      while (VCount >= 160)
         VCount = REG_VCOUNT;

      // wait until the redraw has finished
      VCount = REG_VCOUNT;
      while (VCount < 160)
         VCount = REG_VCOUNT;

      NumSyncs--;
   }
   while (NumSyncs > 0);
}

//指定时间段的淡入淡出,fadeIN=0为淡出（效果增大），否则淡入（效果减小）
void FadeInOut_mine(u8 fadeIN, u8 colorWhite, u32 time)
{
	if(colorWhite==0)
		REG_BLDMOD = 0xFFFF;
	else
		REG_BLDMOD = 0xFFBF;
	for(u32 i=0;i<16+1;i++)
	{
		if(fadeIN==0)
			REG_COLEY2 = 0x0800+i;
		else
			REG_COLEY2 = 0x0800+16-i;
		Sleep(time);
	}
}
// draw a 2x2 tile on the specified plane
void DrawTile(int x, int y, int theTile, int Plane)
{
   int Pos, Tile;

   Pos = (y * 64) + x + x;
   Tile = ((theTile - 1) * 4) +1;

   if (Plane == 1)
   {
      map1[Pos] = Tile;
      map1[Pos+1] = Tile+1;
      map1[Pos+32] = Tile+2;
      map1[Pos+33] = Tile+3;
   }
   else
   {
      map0[Pos] = Tile;
      map0[Pos+1] = Tile+1;
      map0[Pos+32] = Tile+2;
      map0[Pos+33] = Tile+3;
   }
}


// returns the value of the 2x2 tile at the specified location
int GetTile(int x, int y)
{
   int Pos, Tile;

   // multiple x * 2 and y * 64 to give screen coords
   Pos = (y * 64) + x + x;

   Tile = map1[Pos];

   Tile = ((Tile-1) / 4) + 1;
   return Tile;
}


// display the title screen until start is pressed
int DoTitleScreen(void)
{
   volatile u16 Button;
   int i, x, y;

   for (i = 0; i < 256; i++)              // copy colour palettes
      tpal[i] = TitlePalette[i];

   for (i = 0; i < ((30*20)+1) * 64; i++) // copy tiledata for full screen
      tiles[i] = TitleData[i];

   for (x = 0; x < 30; x ++)
      for (y = 0; y < 20; y++)
         map0[x + (y * 32)] = x + (y * 30) +1;

   Button = REG_P1;
   
   /*
   while (1)
   {
	   if((Button & J_START) != 0)
		   return 0;
	   //if((Button & J_B) !=0)
		   return 1;
   }
   Button = REG_P1;
   */
}


// draw the screen full of tiles.
void DrawScreen(void)
{
   int x, y;

   for (y = 0; y < MAX_Y; y ++)
   {
      for (x = 0; x < MAX_X; x ++)
      {
         DrawTile(x, y+MINEFIELD_OFFSET, TILECHAR, 1);
      }
   }
}


void EraseCursor()
{
   int Pos;
   
   Pos = ((OldYPos + MINEFIELD_OFFSET) * 64) + (OldXPos * 2);
   map0[Pos]      = 0;
   map0[Pos+1]    = 0;
   map0[Pos+32]   = 0;
   map0[Pos+33]   = 0;
}


void DrawCursor(int x, int y)
{
   int Pos;

   // erase old cursor
   EraseCursor();
   OldYPos = y;
   OldXPos = x;
   
   // draw new cursor
   Pos = ((y + MINEFIELD_OFFSET) * 64) + (x * 2);
   map0[Pos]      = TILECURSOR;
   map0[Pos+1]    = TILECURSOR+1;
   map0[Pos+32]   = TILECURSOR+2;
   map0[Pos+33]   = TILECURSOR+3;
}




// random stuff extracted from an email from 'ninge1'
#define RAND_MAX 32767
volatile s32 RAND_RandomData;

void SeedRandom(void)
{
   RAND_RandomData = REG_VCOUNT;
}


s32 RAND(s32 Value)
{
   RAND_RandomData *= 20077;
   RAND_RandomData += 12345;

   return ((((RAND_RandomData >> 16) & RAND_MAX) * Value) >> 15);
}


// build a map (mines and values)
void CreateMap(void)
{
   int x, y, mines;
         
   // first clear the array
   for (y = 0; y < MAX_Y; y ++)
      for (x = 0; x < MAX_X; x ++)
         MineField[x][y] = 0;

   for (mines = 0; mines < MAX_MINES; mines++)
   {
      int xrand, yrand;

      // get a space with no mines already on it.
      do
      {
         xrand = RAND(MAX_X);
         yrand = RAND(MAX_Y);
      }
      while (MineField[xrand][yrand] == MINECHAR);

      MineField[xrand][yrand] = MINECHAR;

      // complete the adjacent numbers
      if (xrand > 0 && yrand > 0 && MineField[xrand-1][yrand-1] != MINECHAR)
         MineField[xrand-1][yrand-1]++;
      if (yrand > 0 && MineField[xrand][yrand-1] != MINECHAR)
         MineField[xrand][yrand-1]++;
      if (xrand < MAX_X-1 && yrand > 0 && MineField[xrand+1][yrand-1] != MINECHAR)
         MineField[xrand+1][yrand-1]++;
      if (xrand > 0 && MineField[xrand-1][yrand] != MINECHAR)
         MineField[xrand-1][yrand]++;
      if (xrand < MAX_X-1 && MineField[xrand+1][yrand] != MINECHAR)
         MineField[xrand+1][yrand]++;
      if (xrand > 0 && yrand < MAX_Y-1 && MineField[xrand-1][yrand+1] != MINECHAR)
         MineField[xrand-1][yrand+1]++;
      if (yrand < MAX_Y-1 && MineField[xrand][yrand+1] != MINECHAR)
         MineField[xrand][yrand+1]++;
      if (xrand < MAX_X-1 && yrand < MAX_Y-1 && MineField[xrand+1][yrand+1] != MINECHAR)
         MineField[xrand+1][yrand+1]++;
   }
}


// display the map on the screen
void ShowMap(void)
{
   int x, y;

   for (y = 0; y < MAX_Y; y ++)
   {
      for (x = 0; x < MAX_X; x ++)
      {
         switch(MineField[x][y])
         {
         case 0:
            DrawTile(x, y + MINEFIELD_OFFSET, BLANKCHAR, 1);
            break;
         case MINECHAR:
            DrawTile(x, y + MINEFIELD_OFFSET, MINECHAR, 1);
            break;
         default: // number then
            DrawTile(x, y + MINEFIELD_OFFSET, MineField[x][y]+3, 1);
            break;
         }
      }
   }
}


void DrawStatus(void)
{
   int tens, units;
   
   DrawTile(6, 0, STATUSFLAG, 1);

   tens = FlagCounter / 10;
   units = FlagCounter % 10;

   DrawTile(7, 0, NUMBERCHARS + tens, 1);
   DrawTile(8, 0, NUMBERCHARS + units, 1);
}




void DrawTileAndFlagCheck(int x, int y, int theTile, int Plane)
{
   int Tile;
   
   Tile = GetTile(x, y); // minefield_offset already passed in
   if (Tile == FLAGCHAR)
   {
      FlagCounter++;
      DrawStatus();
   }

   DrawTile(x, y, theTile, Plane);

   // increase our count of how many tiles have been removed.
   TileCount++;
}



void DoSearch(int x, int y)
{
   int Tile;

   // render char
   Tile = MineField[x][y];
   DrawTileAndFlagCheck(x, y + MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);

   // check up
   if (y > 0)
   {
      Tile = GetTile(x, y - 1 + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x][y-1];

         if (Tile == 0)
            DoSearch(x, y - 1);
         else
            DrawTileAndFlagCheck(x, y - 1+ MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }
         
   // check down
   if (y < MAX_Y -1)
   {
      Tile = GetTile(x, y + 1 + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x][y+1];

         if (Tile == 0)
            DoSearch(x, y + 1);
         else
            DrawTileAndFlagCheck(x, y + 1+ MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }

   // check left
   if (x > 0)
   {
      Tile = GetTile(x - 1, y + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x-1][y];

         if (Tile == 0)
            DoSearch(x-1, y);
         else
            DrawTileAndFlagCheck(x-1, y + MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }

   // check right
   if (x < MAX_X -1)
   {
      Tile = GetTile(x+1, y + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x+1][y];

         if (Tile == 0)
            DoSearch(x+1, y);
         else
            DrawTileAndFlagCheck(x+1, y + MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }
   
   // check up left
   if (y > 0 && x > 0)
   {
      Tile = GetTile(x-1, y - 1 + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x-1][y-1];

         if (Tile == 0)
            DoSearch(x-1, y - 1);
         else
            DrawTileAndFlagCheck(x-1, y - 1+ MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }

      // check up right
   if (y > 0 && x < MAX_X -1)
   {
      Tile = GetTile(x+1, y - 1 + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x+1][y-1];

         if (Tile == 0)
            DoSearch(x+1, y - 1);
         else
            DrawTileAndFlagCheck(x+1, y - 1+ MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }

   // check down left
   if (y < MAX_Y -1 && x > 0)
   {
      Tile = GetTile(x-1, y + 1 + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x-1][y+1];

         if (Tile == 0)
            DoSearch(x-1, y + 1);
         else
            DrawTileAndFlagCheck(x-1, y + 1+ MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }

   // check down right
   if (y < MAX_Y -1 && x < MAX_X -1)
   {
      Tile = GetTile(x+1, y + 1 + MINEFIELD_OFFSET);
      if (Tile == TILECHAR || Tile == FLAGCHAR)
      {
         Tile = MineField[x+1][y+1];

         if (Tile == 0)
            DoSearch(x+1, y + 1);
         else
            DrawTileAndFlagCheck(x+1, y + 1+ MINEFIELD_OFFSET, Tile + BLANKCHAR, 1);
      }
   }
}


// draws boom tiles given the x and y of the top left
void DrawBoom(int x, int y)
{
   DrawTile(x, y, BOOMTILES, 0);
   DrawTile(x+1, y, BOOMTILES+1, 0);
   DrawTile(x+2, y, BOOMTILES+2, 0);
   DrawTile(x, y+1, BOOMTILES+3, 0);
   DrawTile(x+1, y+1, BOOMTILES+4, 0);
   DrawTile(x+2, y+1, BOOMTILES+5, 0);
   DrawTile(x, y+2, BOOMTILES+6, 0);
   DrawTile(x+1, y+2, BOOMTILES+7, 0);
   DrawTile(x+2, y+2, BOOMTILES+8, 0);
}

// removes boom tiles given the x and y of the top left
void EraseBoom(int x, int y)
{
   int x1, y1;

   for (x1 = 0; x1 < 6; x1 ++)
      for (y1 = 0; y1 < 6; y1 ++)
         map0[x1 +x +x + ((y * 64) + (y1 *32))] = 0;
}

void DoBoom(int x, int y)
{
   int i;
   u16 OldDispCnt;

   y += MINEFIELD_OFFSET;
   
   EraseCursor();
   DrawBoom(x - 1, y - 1);
   
   OldDispCnt = REG_DISPCNT;

   // flash boom
   for (i = 1; i < 7; i ++)
   {
      REG_DISPCNT = BG_MODE_0 | OBJ_1D_MAP | BG1_ENABLE;
      Sleep(i*2);
      REG_DISPCNT = OldDispCnt;
      Sleep(i*2);
   }

   Sleep(10);
}


int RemoveTile(int x, int y)
{
   int Tile;

   // wait for button release
   volatile u16 Button;
   Button = REG_P1;
   while ((Button & J_B) == 0)
      Button = REG_P1;

   
   Tile = GetTile(x, y + MINEFIELD_OFFSET);

   if (Tile != TILECHAR && Tile != FLAGCHAR)
      return 0;

   Tile = MineField[x][y];
   
   if (Tile == 0)
      DoSearch(x, y);
   else
      DrawTileAndFlagCheck(x, y + MINEFIELD_OFFSET, Tile  + BLANKCHAR, 1);
      
   if (Tile == MINECHAR)
   {
      ShowMap();
      DoBoom(x, y);
      return 1;
   }

   return 0;
}


void FlagTile(int x, int y)
{
   // wait for button release
   volatile u16 Button;
   Button = REG_P1;
   while ((Button & J_B) == 0)
      Button = REG_P1;

   if (FlagCounter > 0 && GetTile(x, y + MINEFIELD_OFFSET) == TILECHAR)
   {
      DrawTile(x, y + MINEFIELD_OFFSET, FLAGCHAR, 1);
      FlagCounter --;
      DrawStatus();
   }
   else if (GetTile(x, y + MINEFIELD_OFFSET) == FLAGCHAR)
   {
      DrawTile(x, y + MINEFIELD_OFFSET, TILECHAR, 1);
      FlagCounter ++;
      DrawStatus();
   }
}


// blow up all of the mines
void DoEndSequence()
{
   int x, y;

   EraseCursor();
   ShowMap();
   for (y = 0; y < MAX_Y; y ++)
   {
      for (x = 0; x < MAX_X; x ++)
      {
         if (MineField[x][y] == MINECHAR)
         {
            DrawBoom(x - 1, y - 1 + MINEFIELD_OFFSET);
            Sleep(7);
            EraseBoom(x - 1, y - 1 + MINEFIELD_OFFSET);

            // blank out numbers around mine
            MineField[x][y] = 0;
            if (x > 0 && y > 0 && MineField[x-1][y-1] != MINECHAR)
               MineField[x-1][y-1] = 0;
            if (y > 0 && MineField[x][y-1] != MINECHAR)
               MineField[x][y-1] = 0;
            if (x < MAX_X-1 && y > 0 && MineField[x+1][y-1] != MINECHAR)
               MineField[x+1][y-1] = 0;
            if (x > 0 && MineField[x-1][y] != MINECHAR)
               MineField[x-1][y] = 0;
            if (x < MAX_X-1 && MineField[x+1][y] != MINECHAR)
               MineField[x+1][y] = 0;
            if (x > 0 && y < MAX_Y-1 && MineField[x-1][y+1] != MINECHAR)
               MineField[x-1][y+1] = 0;
            if (y < MAX_Y-1 && MineField[x][y+1] != MINECHAR)
               MineField[x][y+1] = 0;
            if (x < MAX_X-1 && y < MAX_Y-1 && MineField[x+1][y+1] != MINECHAR)
               MineField[x+1][y+1] = 0;

            ShowMap();
         }
      }
   }
}
   


// clear both tile maps
void ClearPlayFields()
{
   int i;

   for (i = 0; i < (32*32); i ++)
   {
      map0[i] = 0;
      map1[i] = 0;
   }
}

// main function ...
int gameMine()
{
   int i;
   int InPlay;
   int XPos, YPos;
   
   tpal  =REG_PALETTE_BASE;           // background palette pointer
   tiles =REG_TILE_BASE + 0x4000;     // tiles pointer
   map0  =REG_TILEMAP_BASE;           // tile map pointer
   map1  =REG_TILEMAP_BASE + 0x800;   // tile map for background1

   // oops magic numbers -
   // 0x80 represents 1 palette of 256 colours
   // 0x04 character set for this plane is stored at TILEBASE + 0x4000
   // 0x200 the tilemap (screen memory) for this plane is stored at TILEMAPBASE + 0x800
	  FadeInOut_mine(0,0,5);
   REG_BG0CNT =  0x84;                  // tiles @ TILEBASE+0x4000, tilemap @ TILEMAPBASE
   REG_BG1CNT =  0x284;                 // tiles @ TILEBASE+0x4000, tilemap @ TILEMAPBASE + 0x800
   REG_DISPCNT = BG_MODE_0 | OBJ_1D_MAP | BG0_ENABLE | BG1_ENABLE;

   //while (1)
   {
      ClearPlayFields();
      //DoTitleScreen();
      ClearPlayFields();

      for (i = 0; i < 256; i++)              // copy colour palettes
         tpal[i] = TilePalette[i];

      for (i = 0; i < NUM_TILES*64; i++)             // copy tiledata
         tiles[i] = TileData[i];


      
      // reset variables
      XPos = MAX_X / 2;
      YPos = MAX_Y / 2;
      OldXPos = XPos;
      OldYPos = YPos;
      FlagCounter = 20;
      TileCount = 0;

      DrawStatus();
      DrawScreen();
      DrawCursor(XPos, YPos);

      SeedRandom();
      CreateMap();

      InPlay = 1;
	  FadeInOut_mine(1,0,5);
      while(InPlay)
      {
         int JoystickMoved;
         JoystickMoved = 0;

         if ((REG_P1 & J_UP) == 0)
         {
               if (YPos > 0)
               {
                  YPos--;
                  DrawCursor(XPos, YPos);
                  JoystickMoved = 1;
  				  Sleep(8);
             }
         }
         if ((REG_P1 & J_DOWN) == 0)
         {
               if (YPos < MAX_Y - 1)
               {
                  YPos ++;
                  DrawCursor(XPos, YPos);
                  JoystickMoved = 1;
  				  Sleep(8);
               }
         }
         if ((REG_P1 & J_LEFT) == 0)
         {
               if (XPos > 0)
               {
                  XPos--;
                  DrawCursor(XPos, YPos);
                  JoystickMoved = 1;
  				  Sleep(8);
               }
         }
         if ((REG_P1 & J_RIGHT) == 0)
         {
               if (XPos < MAX_X - 1)
               {
                  XPos++;
                  DrawCursor(XPos, YPos);
                  JoystickMoved = 1;
  				  Sleep(8);
               }
         }

         if ((REG_P1 & J_B) == 0) // flag tile
         {
            FlagTile(XPos, YPos);
            JoystickMoved = 1;
         }
            
         if ((REG_P1 & J_A) == 0) // remove tile
         {
            if (RemoveTile(XPos, YPos) == 1) // mine hit!
               InPlay = 0; // exit
            JoystickMoved = 1;

            // check for level complete
            if (TileCount == (MAX_Y * MAX_X) - MAX_MINES)
            {
               DoEndSequence();
               InPlay = 0; // exit
            }
         }
         
         if ((REG_P1 & J_START) == 0) // back to title screen
             InPlay = 0;

         // if there was any user entry cause a slow down
         if (JoystickMoved)
            Sleep(5);
      }
   }
}
