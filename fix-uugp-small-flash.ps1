# Fix problem #2 in CHARGERS.cpp
#
# UUGP should be blocked from selection through
# supported_charger_types(), not by returning "None"
# from name_for_charger_type().
#
# This script only changes name_for_charger_type().
#
# File:
# Software/src/charger/CHARGERS.cpp

$ErrorActionPreference = "Stop"

$file = Join-Path $PSScriptRoot "Software/src/charger/CHARGERS.cpp"

if (-not (Test-Path $file)) {
    throw "Could not find: $file"
}

$content = Get-Content $file -Raw

# ------------------------------------------------------------
# Expected current UUGP block
# ------------------------------------------------------------

$old = @'
    case ChargerType::UUGP:
#ifndef SMALL_FLASH_DEVICE
      return UUGPCharger::Name;
#else
      return "None";
#endif
'@

# ------------------------------------------------------------
# Desired UUGP block
# ------------------------------------------------------------

$new = @'
    case ChargerType::UUGP:
      return UUGPCharger::Name;
'@

# ------------------------------------------------------------
# Verify expected code exists
# ------------------------------------------------------------

if (-not $content.Contains($old)) {
    throw @"
Could not find the expected UUGP block in CHARGERS.cpp.

No changes were made.

Please check:
    git diff -- Software/src/charger/CHARGERS.cpp
"@
}

# ------------------------------------------------------------
# Create backup
# ------------------------------------------------------------

$backup = "$file.bak"

if (Test-Path $backup) {
    Remove-Item $backup -Force
}

Copy-Item $file $backup

# ------------------------------------------------------------
# Apply change
# ------------------------------------------------------------

$content = $content.Replace($old, $new)

Set-Content -Path $file -Value $content -NoNewline

Write-Host ""
Write-Host "SUCCESS: Problem #2 fixed." -ForegroundColor Green
Write-Host ""
Write-Host "UUGP is now blocked from selection through"
Write-Host "supported_charger_types() on SMALL_FLASH_DEVICE."
Write-Host ""
Write-Host "name_for_charger_type() now always returns UUGPCharger::Name."
Write-Host ""
Write-Host "Backup created:"
Write-Host "  $backup"
Write-Host ""
Write-Host "Review the change with:"
Write-Host "  git diff -- Software/src/charger/CHARGERS.cpp"
Write-Host ""
