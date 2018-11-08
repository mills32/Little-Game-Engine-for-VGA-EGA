rem compile c files
tcc -Ic:\tc\include -ml -c main.c n_engine.c n_gus.c
rem link
tlink c0l main.obj n_engine.obj n_gus.obj ,play.exe,,emu mathl cl -Lc:\tc\lib
del *.MAP
del *.OBJ