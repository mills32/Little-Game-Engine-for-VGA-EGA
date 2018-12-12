rem required Turbo C installed at C:\TC
rem compile c files
cd src
tcc -G -O -Ic:\tc\include -ml -c main.c
rem link
tlink c0l.obj main.obj lt_lib.lib,play.exe,,emu mathl cl -Lc:\tc\lib
copy play.exe ..
del *.OBJ
del *.MAP
del play.exe
cd..