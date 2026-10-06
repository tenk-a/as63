del *.bak *.obj
wcl -ml -wx -Ox -DNDEBUG -DSMALL_HOST -D__DOS__ -Fe=..\bin\as63.exe ..\src\as63.c ..\src\gencode.c ..\src\optab.c ..\src\macro.c
