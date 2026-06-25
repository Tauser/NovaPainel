# NovaPainel - tools/scripts/idf_env.ps1
# Activates the ESP-IDF v5.5.4 PowerShell environment (idf.py, esptool.py,
# etc.) and cd's into firmware/, so you don't have to retype the Espressif
# profile path every session.
#
# MUST be dot-sourced (not just run), like the Espressif profile itself,
# otherwise the env vars/functions/aliases it sets only live for the
# subprocess and vanish immediately:
#   . tools\scripts\idf_env.ps1
#
# After this, idf.py / esptool.py work directly, e.g.:
#   idf.py build
#   idf.py -p COM8 flash
#   esptool.py --port COM8 chip_id

$EspressifProfile = "C:\Espressif\tools\Microsoft.v5.5.4.PowerShell_profile.ps1"

if (-not (Test-Path -LiteralPath $EspressifProfile)) {
    throw "ESP-IDF profile not found: $EspressifProfile (wrong machine, or IDF not installed at C:\Espressif?)"
}

. $EspressifProfile *> $null

$FirmwareDir = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "firmware"
Set-Location -LiteralPath $FirmwareDir

Write-Host "NovaPainel: ESP-IDF v5.5.4 environment active, cwd=$FirmwareDir" -ForegroundColor Green
