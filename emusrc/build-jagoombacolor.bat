cd jagoomba

set PATH=C:\devkitPro\msys\bin;C:\devkitPro\devkitARM\bin;%PATH%
set DEVKITARM=/c/devkitPro/devkitARM
set DEVKITPRO=/c/devkitPro
set LIBGBA=/c/devkitPro/libgba

make
ren .\build\font.lz77.o font.o
ren .\build\fontpal.bin.o fontpal.o
make

cd ..
copy .\jagoomba\jagoombacolor.gba goomba.gba
bin2c -o goomba.h goomba.gba
copy goomba.h ..\source\goomba.h

rem >error.txt 2>&1
pause