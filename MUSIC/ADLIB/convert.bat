rem for /f %%a IN ('dir /b /s "C:\Mis_Mega_Cosas\MSDOS_GE\lt_eng_x\music\adlib\*.dro"') do call dro2imf -in %%a -out "%%a".imf -rate 60 -type 0
dro2imf -in letsg.dro -out temp.imf -rate 50 -type 0
imfcrush temp.imf letsg.imf /keepfirst
del temp.imf
pause 
