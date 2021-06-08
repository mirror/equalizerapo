FOR /F "tokens=*" %%g IN ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -products * -property installationPath') do (SET vspath=%%g)
call "%vspath%\Common7\Tools\VsDevCmd.bat"
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=win32 /t:rebuild /m
if %ERRORLEVEL% GEQ 1 goto done
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=x64 /t:rebuild /m
if %ERRORLEVEL% GEQ 1 goto done

if not exist build-Editor-Desktop_Qt_5_15_2_MSVC2019_32bit mkdir build-Editor-Desktop_Qt_5_15_2_MSVC2019_32bit
cd build-Editor-Desktop_Qt_5_15_2_MSVC2019_32bit
call "%vspath%\VC\Auxiliary\Build\vcvarsall.bat" x86
"C:\Qt\5.15.2\msvc2019\bin\qmake.exe" ..\Editor\Editor.pro -r "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

if not exist build-Editor-Desktop_Qt_5_15_2_MSVC2019_64bit mkdir build-Editor-Desktop_Qt_5_15_2_MSVC2019_64bit
cd build-Editor-Desktop_Qt_5_15_2_MSVC2019_64bit
call "%vspath%\VC\Auxiliary\Build\vcvarsall.bat" amd64
"C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe" ..\Editor\Editor.pro -r "CONFIG+=release"
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
