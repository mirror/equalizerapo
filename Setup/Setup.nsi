!include "MUI2.nsh"
!include "NSISpcre.nsh"

!insertmacro REReplace

!searchparse /file ..\version.h `#define MAJOR ` MAJOR
!searchparse /file ..\version.h `#define MINOR ` MINOR

!define REGPATH "Software\EqualizerAPO"
!define UNINST_REGPATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\EqualizerAPO"

;--------------------------------
;General

  ;Name and file
  Name "Equalizer APO ${MAJOR}.${MINOR}"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin
  
;--------------------------------
;Variables

  Var StartMenuFolder
  Var OldStartMenuFolder
  
;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_COMPONENTSPAGE_NODESC

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE ..\License.txt
  !insertmacro MUI_PAGE_DIRECTORY
  
;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY ${REGPATH} 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
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
;Macros

!macro RenameAndDelete path
  IfFileExists "${path}" 0 +3
    Rename "${path}" "${path}.old"
    Delete /REBOOTOK "${path}.old"
!macroend

;--------------------------------
;Functions
Function .onInit
  SetRegView 64
  ;Get installation folder from registry if available
  ReadRegStr $INSTDIR HKLM ${REGPATH} "InstallPath"

  ;Use default installation folder otherwise
  StrCmp $INSTDIR "" 0 +2
    StrCpy $INSTDIR "$PROGRAMFILES64\EqualizerAPO"
    
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  ;Try to replace version number in start menu folder
  ${REReplace} $0 "Equalizer APO [0-9]+\.[0-9]+" "$StartMenuFolder" "Equalizer APO ${MAJOR}.${MINOR}" 1
  StrCmp $0 "" +2 0
    StrCpy $StartMenuFolder "$0"
  
  Call initCheck
FunctionEnd

;--------------------------------
;Installer Sections

Section "Install" SecInstall
  SetOutPath "$INSTDIR"
  SetRebootFlag true

  ;Possibly remove files from previous installation
  !insertmacro MUI_STARTMENU_GETFOLDER Application $OldStartMenuFolder
  RMDir /r "$SMPROGRAMS\$OldStartMenuFolder"

  ;Rename before delete as these files may be in use
  !insertmacro RenameAndDelete "$INSTDIR\EqualizerAPO.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp100.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcr100.dll"
    
  File "${BINPATH}\EqualizerAPO.dll"
  File "${BINPATH}\Configurator.exe"
  File "${BINPATH}\Benchmark.exe"
  
  File "${LIBPATH}\libsndfile-1.dll"
  File "${LIBPATH}\msvcp100.dll"
  File "${LIBPATH}\msvcr100.dll"
  
  File "Configuration tutorial (online).url"
  
  CreateDirectory "$INSTDIR\config"
  
  SetOverwrite off
  File /oname=config\config.txt "config\config.txt"
  File /oname=config\example.txt "config\example.txt"
  File /oname=config\demo.txt "config\demo.txt"
  File /oname=config\multichannel.txt "config\multichannel.txt"
  SetOverwrite on

  ;Grant write access to the config directory for all users
  AccessControl::GrantOnFile "$INSTDIR\config" "(S-1-5-32-545)" "FullAccess"

  WriteRegStr HKLM ${REGPATH} "InstallPath" "$INSTDIR"
  WriteRegStr HKLM ${REGPATH} "ConfigPath" "$INSTDIR\config"
  WriteRegStr HKLM ${REGPATH} "EnableTrace" "false"
  
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configuration tutorial (online).lnk" "$INSTDIR\Configuration tutorial (online).url"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configurator.lnk" "$INSTDIR\Configurator.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Benchmark.lnk" "$INSTDIR\Benchmark.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
  
  WriteRegStr HKLM ${UNINST_REGPATH} "DisplayName" "Equalizer APO"
  WriteRegStr HKLM ${UNINST_REGPATH} "DisplayVersion" "${MAJOR}.${MINOR}"
  WriteRegStr HKLM ${UNINST_REGPATH} "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKLM ${UNINST_REGPATH} "NoModify" 1
  WriteRegDWORD HKLM ${UNINST_REGPATH} "NoRepair" 1

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
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  RMDir /r "$SMPROGRAMS\$StartMenuFolder"
  
  Delete "$INSTDIR\Configuration tutorial (online).url"
  
  Delete /REBOOTOK "$INSTDIR\msvcr100.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp100.dll"
  Delete "$INSTDIR\libsndfile-1.dll"
  
  Delete "$INSTDIR\Benchmark.exe"
  Delete "$INSTDIR\Configurator.exe"
  Delete /REBOOTOK "$INSTDIR\EqualizerAPO.dll"

  Delete "$INSTDIR\Uninstall.exe"

  ;Only remove if empty
  RMDir /REBOOTOK "$INSTDIR"

  DeleteRegKey HKLM ${UNINST_REGPATH}
  DeleteRegKey /ifempty HKLM ${REGPATH}

SectionEnd
