for /f %%a IN ('dir /b /s "C:\Mis_Mega_Cosas\MSDOS_GE\lt_eng\music\*.dro"') do call dro2imf -in %%a -out "%%a".imf -rate 70 -type 0
rem platfor1.imf platfor11.imf /keepall /fixdro 
pause 
