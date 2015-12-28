; Use the modern user interface
!define MUI_ICON "installer.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP "instsplash.bmp"
!include "MUI2.nsh"

Name "Faint Graphics Editor (ALPHA)"
OutFile "Install Faint.exe"

InstallDir "$PROGRAMFILES\Faint"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "" SecDummy	
$$FILES
WriteUninstaller "$INSTDIR\Uninstall.exe"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint" \
                 "DisplayName" "Faint Graphics Editor"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint" \
                 "DisplayIcon" "$\"$INSTDIR\icon.ico$\""
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint" \
                 "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint" \
                 "Publisher" "Lukas Kemmer"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint" \
                 "DisplayVersion" "$$VERSION"	    	 
WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint" \
                 "EstimatedSize" "$$SIZE"
SectionEnd

Section "Uninstall"
$$UNINSTALLFILES
DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Faint"
SectionEnd
