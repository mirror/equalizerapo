!include "MUI2.nsh"

!searchparse /file ..\version.h `#define MAJOR ` MAJOR
!searchparse /file ..\version.h `#define MINOR ` MINOR

!define REGPATH "Software\EqualizerAPO"

;--------------------------------
;General

  ;Name and file
  Name "Equalizer APO ${MAJOR}.${MINOR}"

  ;Default installation folder
  InstallDir "$PROGRAMFILES64\EqualizerAPO"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_COMPONENTSPAGE_NODESC

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE ..\License.txt
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_COMPONENTS
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "German"

;--------------------------------
;Installer Sections

Section "Install" SecInstall
  SetRegView 64

  SetOutPath "$INSTDIR"
  SetRebootFlag true

  File "${BINPATH}\EqualizerAPO.dll"
  File "${BINPATH}\Configurator.exe"
  File "${BINPATH}\Benchmark.exe"
  
  File "${LIBPATH}\libsndfile-1.dll"
  File "${LIBPATH}\msvcp100.dll"
  File "${LIBPATH}\msvcr100.dll"
  
  CreateDirectory "$INSTDIR\config"
  
  SetOverwrite off
  File /oname=config\config.txt "config\config.txt"
  File /oname=config\example.txt "config\example.txt"
  SetOverwrite on

  ;Grant write access to the config directory for all users
  AccessControl::GrantOnFile "$INSTDIR\config" "(S-1-5-32-545)" "FullAccess"

  WriteRegStr HKLM ${REGPATH} "InstallPath" "$INSTDIR"
  WriteRegStr HKLM ${REGPATH} "ConfigPath" "$INSTDIR\config"
  WriteRegStr HKLM ${REGPATH} "EnableTrace" "false"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Audio" "DisableProtectedAudioDG" 1
  ;RegDLL doesn't work for 64 bit dlls
  ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\EqualizerAPO.dll"'

  ExecWait '"$INSTDIR\Configurator.exe" /i'
  
SectionEnd

;--------------------------------
;Uninstaller Sections

LangString SecRemoveName ${LANG_ENGLISH} "Remove configurations and registry backups"
LangString SecRemoveName ${LANG_GERMAN} "Konfigurationen und Registrierungsbackups entfernen"

Section /o un.$(SecRemoveName)
  
  Delete "$INSTDIR\*.reg"
  RMDir /REBOOTOK /r "$INSTDIR\config"
  
SectionEnd

Section "-un.Uninstall"
  SetRegView 64

  ExecWait '"$INSTDIR\Configurator.exe" /u'
  
  ExecWait '"$SYSDIR\regsvr32.exe" /u /s "$INSTDIR\EqualizerAPO.dll"'
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Audio" "DisableProtectedAudioDG"

  Delete /REBOOTOK "$INSTDIR\msvcr100.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp100.dll"
  Delete "$INSTDIR\libsndfile-1.dll"
  
  Delete "$INSTDIR\Benchmark.exe"
  Delete "$INSTDIR\Configurator.exe"
  Delete /REBOOTOK "$INSTDIR\EqualizerAPO.dll"

  Delete "$INSTDIR\Uninstall.exe"

  ;Only remove if empty
  RMDir /REBOOTOK "$INSTDIR"

  DeleteRegKey /ifempty HKLM ${REGPATH}

SectionEnd
