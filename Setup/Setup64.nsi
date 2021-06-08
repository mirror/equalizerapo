
!define BINPATH "..\x64\Release"
!define BINPATH_EDITOR "..\build-Editor-Desktop_Qt_5_15_2_MSVC2019_64bit\release"
!define LIBPATH "lib64"
!define VCREDIST_URL "https://aka.ms/vs/16/release/vc_redist.x64.exe"

!include "Setup.nsi"

LangString VersionError ${LANG_ENGLISH} "This installer is only supposed to be run on 64-Bit Windows. Please use the 32-Bit installer."
LangString VersionError ${LANG_GERMAN} "Dieses Installationsprogramm kann nur auf einem 64-Bit-Windows verwendet werden. Bitte nutzen Sie die 32-Bit-Version."

Function initCheck
	StrCmp $PROGRAMFILES32 $PROGRAMFILES64 0 +3
	MessageBox MB_OK|MB_ICONSTOP $(VersionError)
	Abort
FunctionEnd

OutFile "EqualizerAPO64-${VERSION}.exe"
