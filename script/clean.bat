@echo off

if exist "build" rmdir /s /q "build"
if exist "program\include\resource.hpp" del "program\include\resource.hpp"
if exist "program\source\resource.cpp" del "program\source\resource.cpp"
