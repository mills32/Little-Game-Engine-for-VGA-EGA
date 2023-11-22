rem requires Turbo C installed at C:\TC
tcc -G -O2 -Ic:\tc\include -ml -c main.c 
tlink c0l.obj main.obj lt_lib.lib,play.exe,, cl -Lc:\tc\lib
@echo off
copy play.exe ..
del *.OBJ
del *.MAP
@echo on
