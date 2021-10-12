rem requires Turbo C installed at C:\TC
cd src
tcc -G -O2 -Ic:\tc\include -ml -c lt_sys.c lt_key.c lt_gfx.c lt_spr.c lt_sound.c main.c 
del *.LIB
tlib lt_lib.lib +lt_sys.obj+lt_key.obj+lt_gfx.obj+lt_spr.obj+lt_sound.obj
tlink c0l.obj main.obj lt_lib.lib,play.exe,, cl -Lc:\tc\lib
@echo off
copy play.exe ..
del *.OBJ
del *.MAP
del play.exe
cd..
@echo on
