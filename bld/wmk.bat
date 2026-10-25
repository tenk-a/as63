del *.bak *.obj
if not exist ..\bin mkdir ..\bin
wcl386 -wx -Ox -DNDEBUG -Fe=..\bin\as63.exe ..\src\as63.c ..\src\gencode.c ..\src\macro.c ..\src\optab.c ..\src\objout.c
