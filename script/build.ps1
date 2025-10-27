if (!(Test-Path "build")) { New-Item -ItemType Directory -Path "build" | Out-Null }
cl /nologo /EHsc /std:c++20 /O2 /Fobuild\ /c csb.cpp
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
link /NOLOGO /MACHINE:$Env:PROCESSOR_ARCHITECTURE /OUT:build\csb.exe build\csb.obj
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
.\build\csb.exe
