call "%VS120COMNTOOLS%\vsvars32.bat"
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=win32 /t:rebuild
if %ERRORLEVEL% GEQ 1 goto done
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=x64 /t:rebuild
if %ERRORLEVEL% GEQ 1 goto done

if not exist build-Editor-Desktop_Qt_5_7_MSVC2013_32bit-Release mkdir build-Editor-Desktop_Qt_5_7_MSVC2013_32bit-Release
cd build-Editor-Desktop_Qt_5_7_MSVC2013_32bit-Release
call "%VCINSTALLDIR%vcvarsall.bat" x86
"C:\Qt\5.7\msvc2013\bin\qmake.exe" ..\Editor\Editor.pro -r -spec win32-msvc2013 "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

if not exist build-Editor-Desktop_Qt_5_7_MSVC2013_64bit-Release mkdir build-Editor-Desktop_Qt_5_7_MSVC2013_64bit-Release
cd build-Editor-Desktop_Qt_5_7_MSVC2013_64bit-Release
call "%VCINSTALLDIR%vcvarsall.bat" amd64
"C:\Qt\5.7\msvc2013_64\bin\qmake.exe" ..\Editor\Editor.pro -r -spec win32-msvc2013 "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

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
