rem compile c files
tcc -Ic:\tc\include -ml -c main.c n_engine.c
rem compile assembly files
tasm n_test.asm;
rem link
tlink c0l main.obj n_engine.obj n_test.obj,play.exe,,emu mathl cl -Lc:\tc\lib
del *.MAP
del *.OBJ