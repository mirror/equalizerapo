call "%VS100COMNTOOLS%\vsvars32.bat"
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=win32 /t:rebuild
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=x64 /t:rebuild

cd Setup
"%ProgramFiles%\NSIS\makensis.exe" Setup32.nsi
"%ProgramFiles%\NSIS\makensis.exe" Setup64.nsi
cd..
