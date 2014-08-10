call "%VS100COMNTOOLS%\vsvars32.bat"
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=win32 /t:rebuild
if %ERRORLEVEL% GEQ 1 goto done
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=x64 /t:rebuild
if %ERRORLEVEL% GEQ 1 goto done

if defined ProgramFiles(x86) (
	set nsis="%ProgramFiles(x86)%\NSIS\makensis.exe"
) else (
	set nsis="%ProgramFiles%\NSIS\makensis.exe"
)

cd Setup
%nsis% Setup32.nsi
if %ERRORLEVEL% GEQ 1 goto done
%nsis% Setup64.nsi
cd..

:done
pause
