rem for /f %%a IN ('dir /b /s "C:\Mis_Mega_Cosas\MSDOS_GE\lt_eng_x\music\adlib\*.dro"') do call dro2imf -in %%a -out "%%a".imf -rate 60 -type 0
dro2imf -in yellow.dro -out yellow.imf -rate 50 -type 0
rem atwist.imf atwist1.imf /keepall /fixdro 
pause 
