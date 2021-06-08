
!define BINPATH "..\Release"
!define BINPATH_EDITOR "..\build-Editor-Desktop_Qt_5_15_2_MSVC2019_32bit\release"
!define LIBPATH "lib32"
!define VCREDIST_URL "https://aka.ms/vs/16/release/vc_redist.x86.exe"

!include "Setup.nsi"

LangString VersionError ${LANG_ENGLISH} "This installer is only supposed to be run on 32-Bit Windows. Please use the 64-Bit installer."
LangString VersionError ${LANG_GERMAN} "Dieses Installationsprogramm kann nur auf einem 32-Bit-Windows verwendet werden. Bitte nutzen Sie die 64-Bit-Version."

Function initCheck
	StrCmp $PROGRAMFILES32 $PROGRAMFILES64 +3
	MessageBox MB_OK|MB_ICONSTOP $(VersionError)
	Abort
FunctionEnd

OutFile "EqualizerAPO32-${VERSION}.exe"
