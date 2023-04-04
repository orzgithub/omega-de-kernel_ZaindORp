cd jagoomba

set PATH=C:\devkitPro\msys\bin;C:\devkitPro\devkitARM\bin;%PATH%
set DEVKITARM=/c/devkitPro/devkitARM
set DEVKITPRO=/c/devkitPro
set LIBGBA=/c/devkitPro/libgba

make clean

cd ..

del goomba.gba
del goomba.h

pause