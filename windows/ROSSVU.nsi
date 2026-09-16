; ROSS VU // instalador Windows
; Compilar com:  makensis windows/ROSSVU.nsi
; No Fedora:     sudo dnf install mingw64-nsis

!define NOME    "ROSS VU"
!define VERSAO  "1.4.1"
!define AUTOR   "Mister RickRoss"
!define SITE    "https://misterrickross.com/plugins/ross-vu/"

Name "${NOME} ${VERSAO}"
OutFile "ROSS-VU-${VERSAO}-windows-x64-setup.exe"
Unicode True
Target amd64-unicode
SetCompressor /SOLID lzma
RequestExecutionLevel admin
InstallDir "$PROGRAMFILES64\Common Files\VST3"
ShowInstDetails show

VIProductVersion "1.4.1.0"
VIAddVersionKey "ProductName"     "${NOME}"
VIAddVersionKey "FileDescription" "Medidor VU analogico com balistica normativa"
VIAddVersionKey "FileVersion"     "${VERSAO}"
VIAddVersionKey "CompanyName"     "${AUTOR}"
VIAddVersionKey "LegalCopyright"  "GPL-3.0-or-later"

Page license
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

LicenseData "..\LICENSE.txt"

Section "VST3 (recomendado)" SEC_VST3
  SectionIn RO
  SetOutPath "$PROGRAMFILES64\Common Files\VST3\ROSSVU.vst3\Contents\x86_64-win"
  File "..\..\bin-win\ROSSVU.vst3\Contents\x86_64-win\ROSSVU.vst3"
SectionEnd

Section "CLAP" SEC_CLAP
  SetOutPath "$PROGRAMFILES64\Common Files\CLAP"
  File "..\..\bin-win\ROSSVU.clap"
SectionEnd

Section "-Desinstalador"
  SetOutPath "$PROGRAMFILES64\${AUTOR}\${NOME}"
  WriteUninstaller "$PROGRAMFILES64\${AUTOR}\${NOME}\Uninstall.exe"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ROSSVU" \
                   "DisplayName" "${NOME} ${VERSAO}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ROSSVU" \
                   "UninstallString" "$PROGRAMFILES64\${AUTOR}\${NOME}\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ROSSVU" \
                   "Publisher" "${AUTOR}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ROSSVU" \
                   "URLInfoAbout" "${SITE}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ROSSVU" \
                   "DisplayVersion" "${VERSAO}"
SectionEnd

Section "Uninstall"
  Delete "$PROGRAMFILES64\Common Files\VST3\ROSSVU.vst3\Contents\x86_64-win\ROSSVU.vst3"
  RMDir  "$PROGRAMFILES64\Common Files\VST3\ROSSVU.vst3\Contents\x86_64-win"
  RMDir  "$PROGRAMFILES64\Common Files\VST3\ROSSVU.vst3\Contents"
  RMDir  "$PROGRAMFILES64\Common Files\VST3\ROSSVU.vst3"
  Delete "$PROGRAMFILES64\Common Files\CLAP\ROSSVU.clap"
  Delete "$PROGRAMFILES64\${AUTOR}\${NOME}\Uninstall.exe"
  RMDir  "$PROGRAMFILES64\${AUTOR}\${NOME}"
  RMDir  "$PROGRAMFILES64\${AUTOR}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ROSSVU"
SectionEnd
