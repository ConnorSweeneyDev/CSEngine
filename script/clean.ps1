if (Test-Path "build") { Remove-Item -Recurse -Force "build" }
if (Test-Path "program\include\resource.hpp") { Remove-Item -Force "program\include\resource.hpp" }
if (Test-Path "program\source\resource.cpp") { Remove-Item -Force "program\source\resource.cpp" }
