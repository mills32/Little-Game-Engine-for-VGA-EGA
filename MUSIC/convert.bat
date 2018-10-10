for /f %%a IN ('dir /b /s "C:\Mis_Mega_Cosas\MSDOS_GE\lengine\music\*.dro"') do call dro2imf -in %%a -out "%%a".imf -rate 500 -type 0
rem for /f %%a IN ('dir /b /s "C:\Mis_Mega_Cosas\MSDOS_GE\lengine\music\*.dro"') do call imfcrush "%%a".timf %%a.imf /keepall /fixdro 
rem for /f %%a IN ('dir /b /s "C:\Mis_Mega_Cosas\MSDOS_GE\lengine\music\*.dro"') do del "%%a".timf
pause 
