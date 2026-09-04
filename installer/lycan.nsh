!macro customInstall
  ; Force the user-facing shortcuts to launch the packaged Electron executable.
  ; This avoids accidentally creating a shortcut to a frontend asset.
  CreateShortCut "$DESKTOP\LYCAN OS.lnk" "$INSTDIR\LYCAN.exe" "" "$INSTDIR\LYCAN.exe" 0 SW_SHOWNORMAL
  CreateDirectory "$SMPROGRAMS\LYCAN OS"
  CreateShortCut "$SMPROGRAMS\LYCAN OS\LYCAN OS.lnk" "$INSTDIR\LYCAN.exe" "" "$INSTDIR\LYCAN.exe" 0 SW_SHOWNORMAL
!macroend

!macro customUnInstall
  Delete "$DESKTOP\LYCAN OS.lnk"
  Delete "$SMPROGRAMS\LYCAN OS\LYCAN OS.lnk"
  RMDir "$SMPROGRAMS\LYCAN OS"
!macroend
