::@echo off
setlocal

pushd %~dp0
set "result=0"

:: 6809
call :ase -8 -lm6809.lst -om6809.o m6809.s
call :ase -8 -9 -z -mtst_undoc6809 -o tst_undoc6809.s
call :ase -8 -9 -mtst_undoc6809 -o tst_undoc6809.s
call :asm -8 -ltst68x.lst -otst68x.o tst68x.s
echo:

:: 6309
call :ase -lm6309.lst -om6309.o m6309.s
call :ase -9    -mtst_undoc6309 -o tst_undoc6309.s
call :ase -9 -z -mtst_undoc6309 -o tst_undoc6309.s
call :asm -j -i. -otst_inc.o tst_inc.s
call :ase -otst63x.o tst63x.s
echo:

:: FLEX binary.
call :ase -q -xtst_flex.cmd tst_flex.s

:: OS-9 modules.
call :ase -9 -i. -mlist -olist.o list.s
call :ase -9 -i. -mplace -oplace.o place.s
echo:

:: 疑似命令テスト...
call :ase -otst_exp2.o tst_exp2.s
call :asm -otst_expr.o tst_expr.s
call :asm -9 -otst_expr-os9.o tst_expr.s
call :ase -otst_if.o tst_if.s
call :ase -j -q -ftst_org.s19 tst_org.s
echo:

:: multi file test. 
call :asm -a -ltst_mf_1.lst tst_mf.s
call :ase -a -ltst_mf.lst   tst_mf.s tst_mf2.s

goto END

:asm
@echo [as63] %*
..\bin\as63 %*
echo:
exit /b 0

:ase
@echo [as63] %*
..\bin\as63 %*
if errorlevel 1 set "result=1"
echo:
exit /b 0

:END
popd
@if "%result%"=="1" echo test fail
exit /b %result%
