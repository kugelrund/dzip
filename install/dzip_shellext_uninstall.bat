@ net session >nul 2>&1
@ if not %ERRORLEVEL% == 0 (
    @echo This script requires admin rights. Please restart as admin . . .
    @pause
    exit 1
)

reg delete "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\SystemFileAssociations\.dz\shell\dzip" /f
del "%appdata%\Microsoft\Windows\SendTo\Dzip.lnk"
