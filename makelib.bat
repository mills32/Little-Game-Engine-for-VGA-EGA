rem requires Turbo C installed at C:\TC
tcc -G -O2 -Ic:\tc\include -ml -c lt_sys.c lt_key.c lt_gfx.c lt_spr.c lt_sound.c
@echo off
del *.LIB
@echo on
tlib lt_lib.lib +lt_sys.obj+lt_key.obj+lt_gfx.obj+lt_spr.obj+lt_sound.obj
@echo off
del *.OBJ
@echo on

