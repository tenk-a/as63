@echo off
setlocal

rem This file is intended to be run from anywhere; all paths are test\ relative.
pushd "%~dp0" || exit /b 1
set "result=0"

set "as63=..\bin\as63"

rem OS-9 modules.  defs and defsfile are included through $INC.
echo:
echo [as63] list.s -9 -i. -mlist -olist.o
%as63% -j -9 -i. -mlist -olist.o list.s
if errorlevel 1 set "result=1"
echo [as63] place.s -9 -i. -mplace -oplace.o
%as63% -j -9 -i. -mplace -oplace.o place.s
if errorlevel 1 set "result=1"

rem Expression/conditional tests (tst_expr deliberately contains error cases).
echo:
echo [as63] tst_exp2.s -otst_exp2.o
%as63% -j -otst_exp2.o tst_exp2.s
if errorlevel 1 set "result=1"
echo:
echo [as63] tst_expr.s -otst_expr.o
%as63% -j -otst_expr.o tst_expr.s
if errorlevel 1 set "result=1"
echo:
echo [as63] tst_expr.s -9 -otst_expr-os9.o
%as63% -j -9 -otst_expr-os9.o tst_expr.s
if errorlevel 1 set "result=1"
echo:
echo [as63] tst_if.s -otst_if.o
%as63% -j -otst_if.o tst_if.s
if errorlevel 1 set "result=1"

rem Include-path and 6309 instruction tests.
echo:
echo [as63] tst_inc.s -i. -otst_inc.o
%as63% -j -i. -otst_inc.o tst_inc.s
if errorlevel 1 set "result=1"
echo:
echo [as63] tst63x.s -otst63x.o
%as63% -j -otst63x.o tst63x.s
if errorlevel 1 set "result=1"

rem tst68x is specifically the 6809-mode test; keep its listing.
echo:
echo [as63] tst68x.s -8 -ltst68x.lst -otst68x.o
%as63% -j -8 -ltst68x.lst -otst68x.o tst68x.s
if errorlevel 1 set "result=1"

rem FLEX binary output test.
echo:
echo [as63] tst_flex.s -q -xtst_flex.cmd
%as63% -j -q -xtst_flex.cmd tst_flex.s
if errorlevel 1 set "result=1"

rem ORG/RMB gap test: S-Record output requires -q.
echo:
echo [as63] tst_org.s -q -ftst_org.s19
%as63% -j -q -ftst_org.s19 tst_org.s
if errorlevel 1 set "result=1"

rem Multi-file test: tst_mf.s references labels defined in tst_mf2.s.
rem -a is the FCB-data output mode; its default output is tst_mf.oa.
echo:
echo [as63] tst_mf.s tst_mf2.s -a -ltst_mf.lst
%as63% -j -a -ltst_mf.lst tst_mf.s tst_mf2.s
if errorlevel 1 set "result=1"

popd
exit /b %result%
