del *.bak *.obj
cl -utf-8 -O2 -W4 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS -wd4244 -Feas63.exe ..\src\as63.c ..\src\gencode.c ..\src\optab.c ..\src\macro.c
