; Simple NSIS installer template for CRIT
; Expects STAGE_DIR and OUT_DIR defines provided via makensis -DSTAGE_DIR=... -DOUT_DIR=...

!include "MUI2.nsh"

Name "CRIT Plugin"
OutFile "$OUT_DIR\\crit-installer.exe"
InstallDir "$PROGRAMFILES\\CRIT"
ShowInstDetails show

Section "Install"
  SetOutPath "$INSTDIR"
  ; Copy staged files (all contents from STAGE_DIR)
  ; NSIS requires explicit file listings — use a simple recursive copy via nsExec
  nsExec::ExecToStack 'cmd /C robocopy "${STAGE_DIR}" "$INSTDIR" /E /NFL /NDL /NJH /NJS /nc /ns /np'
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\\*.*"
  RMDir /r "$INSTDIR"
SectionEnd
