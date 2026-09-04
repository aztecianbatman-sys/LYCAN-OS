!macro customInstall
  ; electron-builder is configured not to create default shortcuts.
  ; These are the only user-facing shortcuts created by the installer.
  Delete "$DESKTOP\LYCAN OS.lnk"
  Delete "$DESKTOP\LYCAN.lnk"
  Delete "$SMPROGRAMS\LYCAN OS\LYCAN OS.lnk"
  Delete "$SMPROGRAMS\LYCAN OS\LYCAN.lnk"

  IfFileExists "$INSTDIR\LYCAN.exe" 0 lycan_executable_missing
  CreateShortCut "$DESKTOP\LYCAN OS.lnk" "$INSTDIR\LYCAN.exe" "" "$INSTDIR\LYCAN.exe" 0 SW_SHOWNORMAL
  CreateDirectory "$SMPROGRAMS\LYCAN OS"
  CreateShortCut "$SMPROGRAMS\LYCAN OS\LYCAN OS.lnk" "$INSTDIR\LYCAN.exe" "" "$INSTDIR\LYCAN.exe" 0 SW_SHOWNORMAL
  Goto lycan_shortcuts_done

lycan_executable_missing:
  MessageBox MB_ICONSTOP|MB_OK "LYCAN OS could not create its shortcuts because LYCAN.exe was not installed. Please reinstall LYCAN OS."

lycan_shortcuts_done:
!macroend

!macro customUnInstall
  Delete "$DESKTOP\LYCAN OS.lnk"
  Delete "$DESKTOP\LYCAN.lnk"
  Delete "$SMPROGRAMS\LYCAN OS\LYCAN OS.lnk"
  Delete "$SMPROGRAMS\LYCAN OS\LYCAN.lnk"
  RMDir "$SMPROGRAMS\LYCAN OS"
!macroend
