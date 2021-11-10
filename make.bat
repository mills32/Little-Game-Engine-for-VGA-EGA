rem required Turbo C installed at C:\TC
rem compile c files
cd src
tcc -G -O2 -Ic:\tc\include -ml -c lt_sys.c lt_key.c lt_gfx.c lt_spr.c lt_sound.c main.c 
rem tasm /dc lt_sprc.asm
rem create lib
del *.LIB
tlib lt_lib.lib +lt_sys.obj+lt_key.obj+lt_gfx.obj+lt_spr.obj+lt_sound.obj
rem link
tlink c0l.obj main.obj lt_lib.lib,play.exe,, cl -Lc:\tc\lib
copy play.exe ..
del *.OBJ
del *.MAP
del play.exe
cd..
