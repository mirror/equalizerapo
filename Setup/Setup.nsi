!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "NSISpcre.nsh"
!include "WinVer.nsh"

;Use ANSI because NSISpcre's Unicode version is less optimized
Unicode false
ManifestDPIAware true
;Use more efficient compression
SetCompressor /SOLID lzma

!insertmacro REReplace

!searchparse /file ..\version.h `#define MAJOR ` MAJOR
!searchparse /file ..\version.h `#define MINOR ` MINOR
!searchparse /file ..\version.h `#define REVISION ` REVISION
!if ${REVISION} == 0
!define VERSION ${MAJOR}.${MINOR}
!else
!define VERSION ${MAJOR}.${MINOR}.${REVISION}
!endif

!define REGPATH "Software\EqualizerAPO"
!define UNINST_REGPATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\EqualizerAPO"

;--------------------------------
;General

  ;Name and file
  Name "Equalizer APO ${VERSION}"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin
  
;--------------------------------
;Variables

  Var StartMenuFolder
  Var OldStartMenuFolder
  Var OLDINSTDIR
  
;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_COMPONENTSPAGE_NODESC
  !define MUI_WELCOMEPAGE_TITLE_3LINES

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
  ${If} ${FileExists} "${path}"
    Rename "${path}" "${path}.old"
    Delete /REBOOTOK "${path}.old"
  ${EndIf}
!macroend
  
LangString UCRTError ${LANG_ENGLISH} "Your Windows installation is missing required updates to use this program. Please install remaining Windows updates or the Visual C++ Redistributable for Visual Studio 2015 - 2019.$\n$\nDo you want to download the Visual C++ Redistributable now?"
LangString UCRTError ${LANG_GERMAN} "Ihrer Windows-Installation fehlen benötigte Updates, um dieses Programm zu verwenden. Bitte installieren Sie ausstehende Windows-Updates oder das Visual C++ Redistributable für Visual Studio 2015 - 2019.$\n$\nMöchten Sie jetzt das Visual C++ Redistributable herunterladen?"

;--------------------------------
;Functions
Function .onInit
  !if ${LIBPATH} == "lib64"
    SetRegView 64
  !endif
  ;Get installation folder from registry if available
  ReadRegStr $INSTDIR HKLM ${REGPATH} "InstallPath"

  ;Use default installation folder otherwise
  ${If} $INSTDIR == ""
    StrCpy $INSTDIR "$PROGRAMFILES64\EqualizerAPO"
  ${EndIf}
    
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  ;Try to replace version number in start menu folder
  ${REReplace} $0 "Equalizer APO [0-9]+\.[0-9]+(?:\.[0-9]+)?" "$StartMenuFolder" "Equalizer APO ${VERSION}" 1
  ${If} $0 != ""
    StrCpy $StartMenuFolder "$0"
  ${EndIf}
  
  Call initCheck
  
  ${IfNot} ${AtLeastWin10}
    System::Call 'KERNEL32::LoadLibrary(t "ucrtbase.dll")p.r0'
    ${If} $0 P= 0
      MessageBox MB_YESNO|MB_ICONSTOP $(UCRTError) IDNO skipDownload
	  ExecShell "open" "${VCREDIST_URL}"
	  skipDownload:
	  Abort
    ${EndIf}
  ${EndIf}
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
  !insertmacro RenameAndDelete "$INSTDIR\libfftw3f-3.dll"
  !insertmacro RenameAndDelete "$INSTDIR\libsndfile-1.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp100.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcr100.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp120.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcr120.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp140.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp140_1.dll"
  !insertmacro RenameAndDelete "$INSTDIR\VoicemeeterClient.exe"
  !insertmacro RenameAndDelete "$INSTDIR\vcruntime140.dll"
  !insertmacro RenameAndDelete "$INSTDIR\vcruntime140_1.dll"
    
  File "${BINPATH}\EqualizerAPO.dll"
  File "${BINPATH}\Configurator.exe"
  File "${BINPATH}\Benchmark.exe"
  File "${BINPATH}\VoicemeeterClient.exe"
  
  File "${BINPATH_EDITOR}\Editor.exe"
  
  File "${LIBPATH}\libfftw3f-3.dll"
  File "${LIBPATH}\libsndfile-1.dll"
  File "${LIBPATH}\msvcp140.dll"
  File "${LIBPATH}\msvcp140_1.dll"
  File "${LIBPATH}\Qt5Core.dll"
  File "${LIBPATH}\Qt5Gui.dll"
  File "${LIBPATH}\Qt5Widgets.dll"
  File "${LIBPATH}\vcruntime140.dll"
  !if ${LIBPATH} == "lib64"
	File "${LIBPATH}\vcruntime140_1.dll"
  !endif
  
  CreateDirectory "$INSTDIR\qt"
  CreateDirectory "$INSTDIR\qt\imageformats"
  CreateDirectory "$INSTDIR\qt\platforms"
  CreateDirectory "$INSTDIR\qt\styles"
  
  File /oname=qt\imageformats\qgif.dll "${LIBPATH}\qt\imageformats\qgif.dll"
  File /oname=qt\imageformats\qico.dll "${LIBPATH}\qt\imageformats\qico.dll"
  File /oname=qt\imageformats\qjpeg.dll "${LIBPATH}\qt\imageformats\qjpeg.dll"
  File /oname=qt\platforms\qwindows.dll "${LIBPATH}\qt\platforms\qwindows.dll"
  File /oname=qt\styles\qwindowsvistastyle.dll "${LIBPATH}\qt\styles\qwindowsvistastyle.dll"
  
  File "Configuration tutorial (online).url"
  File "Configuration reference (online).url"
  
  CreateDirectory "$INSTDIR\config"
  CreateDirectory "$INSTDIR\VSTPlugins"
  
  SetOverwrite off
  File /oname=config\config.txt "config\config.txt"
  File /oname=config\example.txt "config\example.txt"
  File /oname=config\demo.txt "config\demo.txt"
  File /oname=config\multichannel.txt "config\multichannel.txt"
  File /oname=config\iir_lowpass.txt "config\iir_lowpass.txt"
  File /oname=config\selective_delay.txt "config\selective_delay.txt"
  SetOverwrite on

  ;Grant write access to the config directory for all users
  AccessControl::GrantOnFile "$INSTDIR\config" "(S-1-5-32-545)" "FullAccess"

  ReadRegStr $OLDINSTDIR HKLM ${REGPATH} "InstallPath"
  WriteRegStr HKLM ${REGPATH} "InstallPath" "$INSTDIR"
  
  ;Write ConfigPath if non-existing or if InstallPath has changed
  ReadRegStr $0 HKLM ${REGPATH} "ConfigPath"
  ${If} $0 == ""
  ${OrIf} $INSTDIR != $OLDINSTDIR
	WriteRegStr HKLM ${REGPATH} "ConfigPath" "$INSTDIR\config"
  ${EndIf}
	
  ReadRegStr $0 HKLM ${REGPATH} "EnableTrace"
  ${If} $0 == ""
	WriteRegStr HKLM ${REGPATH} "EnableTrace" "false"
  ${EndIf}

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configuration Editor.lnk" "$INSTDIR\Editor.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configuration tutorial (online).lnk" "$INSTDIR\Configuration tutorial (online).url"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configuration reference (online).lnk" "$INSTDIR\Configuration reference (online).url"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configurator.lnk" "$INSTDIR\Configurator.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Benchmark.lnk" "$INSTDIR\Benchmark.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
  
  WriteRegStr HKLM ${UNINST_REGPATH} "DisplayName" "Equalizer APO"
  WriteRegStr HKLM ${UNINST_REGPATH} "DisplayVersion" "${VERSION}"
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
  DeleteRegKey HKCU ${REGPATH}
  
SectionEnd

Section "-un.Uninstall"
  !if ${LIBPATH} == "lib64"
	SetRegView 64
  !endif

  ExecWait '"$INSTDIR\Configurator.exe" /u'
  
  ExecWait '"$SYSDIR\regsvr32.exe" /u /s "$INSTDIR\EqualizerAPO.dll"'
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Audio" "DisableProtectedAudioDG"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  RMDir /r "$SMPROGRAMS\$StartMenuFolder"
  
  RMDir "$INSTDIR\VSTPlugins"
  
  Delete "$INSTDIR\Configuration reference (online).url"
  Delete "$INSTDIR\Configuration tutorial (online).url"
  
  RMDir /r "$INSTDIR\qt"
  
  !if ${LIBPATH} == "lib64"
    Delete /REBOOTOK "$INSTDIR\vcruntime140_1.dll"
  !endif
  Delete /REBOOTOK "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\Qt5Widgets.dll"
  Delete "$INSTDIR\Qt5Gui.dll"
  Delete "$INSTDIR\Qt5Core.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp140_1.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp140.dll"
  Delete /REBOOTOK "$INSTDIR\libsndfile-1.dll"
  Delete /REBOOTOK "$INSTDIR\libfftw3f-3.dll"
  Delete "$INSTDIR\Editor.exe"
  
  Delete "$INSTDIR\VoicemeeterClient.exe"
  Delete "$INSTDIR\Benchmark.exe"
  Delete "$INSTDIR\Configurator.exe"
  Delete /REBOOTOK "$INSTDIR\EqualizerAPO.dll"

  Delete "$INSTDIR\Uninstall.exe"

  ;Only remove if empty
  RMDir /REBOOTOK "$INSTDIR"

  DeleteRegKey HKLM ${UNINST_REGPATH}
  DeleteRegKey /ifempty HKLM ${REGPATH}

SectionEnd
