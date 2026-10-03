del *.obj *.bak
cl -utf-8 -O2 -W4 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS -wd4244 -Feas63_vc.exe ..\src\as63.c ..\src\optab.c
