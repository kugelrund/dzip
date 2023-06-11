@ net session >nul 2>&1
@ if not %ERRORLEVEL% == 0 (
    @echo This script requires admin rights. Please restart as admin . . .
    @pause
    exit 1
)

reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\SystemFileAssociations\.dz\shell\dzip" /t REG_SZ /d "Extract with Dzip" /f
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\SystemFileAssociations\.dz\shell\dzip\command" /t REG_SZ /d "\"%~dp0dzip.exe\" -x \"%%1\"" /f
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%appdata%\Microsoft\Windows\SendTo\Dzip.lnk');$s.TargetPath='%~dp0dzip.exe';$s.Save()"
