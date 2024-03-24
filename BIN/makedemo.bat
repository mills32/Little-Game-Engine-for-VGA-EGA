rem requires Turbo C installed at C:\TC
tcc -G -O2 -mc -c main.c
tlink /n c0c.obj main.obj lt_lib.lib,play.exe,, cc -Lc:\tc\lib
@echo off
copy play.exe ..
del *.OBJ
del *.MAP
@echo on
