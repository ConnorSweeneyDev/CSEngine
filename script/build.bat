@echo off

if not exist "build" mkdir "build"
cl /nologo /EHsc /std:c++20 /O2 /Fobuild\ /c csb.cpp
if %errorlevel% neq 0 exit /b %errorlevel%
link /NOLOGO /MACHINE:%PROCESSOR_ARCHITECTURE% /OUT:build\csb.exe build\csb.obj
if %errorlevel% neq 0 exit /b %errorlevel%
build\csb.exe build
