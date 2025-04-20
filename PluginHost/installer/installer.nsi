; Voicemeeter Plugin Host Installer Script
; Created for NSIS (Nullsoft Scriptable Install System)

; --------------------------------
; Includes and Definitions
; --------------------------------

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

; Define basic application information
!define PRODUCT_NAME "Voicemeeter Plugin Host"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Voicemeeter SDK"
!define PRODUCT_WEB_SITE "https://www.vb-audio.com/Voicemeeter/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\VoicemeeterPluginHost.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

; Set compression options
SetCompressor /SOLID lzma

; --------------------------------
; UI Configuration
; --------------------------------

; Modern UI 2 Configuration
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome/Finish Pages
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\modern-wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\modern-wizard.bmp"

; Installer/Uninstaller Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\VoicemeeterPluginHost.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller Pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Set language
!insertmacro MUI_LANGUAGE "English"

; --------------------------------
; Installer Configuration
; --------------------------------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "VoicemeeterPluginHost-Setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

; --------------------------------
; Installer Sections
; --------------------------------

Section "Main Program Files" SecMain
    SetOutPath "$INSTDIR"
    SetOverwrite on
    
    ; Main executable and essential DLLs
    File "..\build\Release\VoicemeeterPluginHost.exe"
    File "..\build\Release\*.dll"
    
    ; Create required directories
    CreateDirectory "$INSTDIR\logs"
    CreateDirectory "$INSTDIR\presets"
    CreateDirectory "$INSTDIR\plugins"
    
    ; Add sample presets
    SetOutPath "$INSTDIR\presets"
    File "..\presets\*.json"
    
    ; Configuration files
    SetOutPath "$INSTDIR"
    File "..\config.json"
    
    ; Documentation
    File "..\README.md"
    File "..\LICENSE"
    File "..\VoicemeeterPluginHostManual.pdf"
    
    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\VoicemeeterPluginHost.exe"
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\VoicemeeterPluginHost.exe"
    
    ; Write registry entries for Add/Remove Programs
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\VoicemeeterPluginHost.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\VoicemeeterPluginHost.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    
    ; Calculate and store installation size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
    
    ; Write uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Visual C++ Redistributable" SecVCRedist
    SetOutPath "$TEMP"
    File "vc_redist.x64.exe"
    ExecWait "$TEMP\vc_redist.x64.exe /passive /norestart"
    Delete "$TEMP\vc_redist.x64.exe"
SectionEnd

Section "Associate Plugin Preset Files" SecFileAssoc
    ; Associate .vmplug files with our application
    WriteRegStr HKCR ".vmplug" "" "VoicemeeterPluginPreset"
    WriteRegStr HKCR "VoicemeeterPluginPreset" "" "Voicemeeter Plugin Host Preset"
    WriteRegStr HKCR "VoicemeeterPluginPreset\DefaultIcon" "" "$INSTDIR\VoicemeeterPluginHost.exe,0"
    WriteRegStr HKCR "VoicemeeterPluginPreset\shell\open\command" "" '"$INSTDIR\VoicemeeterPluginHost.exe" "%1"'
SectionEnd

; --------------------------------
; Uninstaller Sections
; --------------------------------

Section "Uninstall"
    ; Remove program files
    Delete "$INSTDIR\VoicemeeterPluginHost.exe"
    Delete "$INSTDIR\*.dll"
    
    ; Remove configuration files (optional, can be commented out to preserve settings)
    Delete "$INSTDIR\config.json"
    
    ; Remove sample presets
    Delete "$INSTDIR\presets\*.json"
    
    ; Remove documentation
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\VoicemeeterPluginHostManual.pdf"
    
    ; Remove shortcuts
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
    
    ; Remove directories (only if empty)
    RMDir "$INSTDIR\logs"
    RMDir "$INSTDIR\presets"
    RMDir "$INSTDIR\plugins"
    
    ; Remove uninstaller
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    
    ; Remove registry entries
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
    
    ; Remove file associations
    DeleteRegKey HKCR ".vmplug"
    DeleteRegKey HKCR "VoicemeeterPluginPreset"
    
    SetAutoClose true
SectionEnd

; --------------------------------
; Helper Functions
; --------------------------------

Function .onInit
    ; Check if application is already installed
    ReadRegStr $R0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
    StrCmp $R0 "" done
    
    ; Application is already installed, ask if user wants to uninstall first
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
        "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the previous version or `Cancel` to cancel this upgrade." \
        IDOK uninst
    Abort
    
    ; Run the uninstaller
    uninst:
    ClearErrors
    ExecWait '$R0 _?=$INSTDIR'
    
    ; Continue with installation
    done:
FunctionEnd