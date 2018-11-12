rem required Turbo C installed at C:\TC
rem compile c files
cd src
tcc -G -O -Ic:\tc\include -ml -c lt_sys.c lt_gfx.c lt_spr.c lt_key.c lt_adlib.c lt_gus.c main.c
rem create lib
del *.LIB
tlib lt_lib.lib +lt_sys.obj+lt_gfx.obj+lt_spr.obj+lt_key.obj+lt_adlib.obj+lt_gus.obj
rem link
tlink c0l.obj main.obj lt_lib.lib,play.exe,,emu mathl cl -Lc:\tc\lib
copy play.exe ..
del *.OBJ
del *.MAP
del play.exe
cd..
