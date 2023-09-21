@echo off
cd /D "%~dp0../"

rem use relative link to enjine
set UNREAL_PATH=../UnrealEngine
rem find absolute path of symlink to unreal engine because it does not like symlinks
for /f "tokens=2 delims=[]" %%H in  ('dir /al ..\ ^| findstr /i /c:"UnrealEngine"') do (
    set UNREAL_PATH=%%H
)
echo Using engine at %UNREAL_PATH%
%UNREAL_PATH%/Build/BatchFiles/Build.bat %*

