$ErrorActionPreference = "Stop"

$chargersCpp = "Software/src/charger/CHARGERS.cpp"
$canChargerH = "Software/src/charger/CanCharger.h"

function Read-Utf8($path) {
    return [System.IO.File]::ReadAllText((Resolve-Path $path), [System.Text.Encoding]::UTF8)
}

function Write-Utf8NoBom($path, $content) {
    $fullPath = (Resolve-Path $path).Path
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($fullPath, $content, $utf8NoBom)
}

# ------------------------------------------------------------
# CanCharger.h
# ------------------------------------------------------------

$content = Read-Utf8 $canChargerH

$enumPattern = '(?s)enum\s+class\s+ChargerType\s*\{.*?\};'

$enumReplacement = @'
enum class ChargerType {
  None,
  NissanLeaf,
  ChevyVolt,
#ifndef SMALL_FLASH_DEVICE
  UUGP,
#endif
  Highest
};
'@

if ($content -notmatch $enumPattern) {
    throw "Kunne ikke finde ChargerType enum i $canChargerH"
}

$content = [regex]::Replace(
    $content,
    $enumPattern,
    $enumReplacement,
    1
)

Write-Utf8NoBom $canChargerH $content

# ------------------------------------------------------------
# CHARGERS.cpp
# ------------------------------------------------------------

$content = Read-Utf8 $chargersCpp

# supported_charger_types()
$supportedPattern = '(?s)std::vector<ChargerType>\s+supported_charger_types\(\)\s*\{.*?\n\}'

$supportedReplacement = @'
std::vector<ChargerType> supported_charger_types() {
  std::vector<ChargerType> types;

  for (int i = 0; i < (int)ChargerType::Highest; i++) {
    types.push_back((ChargerType)i);
  }

  return types;
}
'@

if ($content -notmatch $supportedPattern) {
    throw "Kunne ikke finde supported_charger_types() i $chargersCpp"
}

$content = [regex]::Replace(
    $content,
    $supportedPattern,
    $supportedReplacement,
    1
)

# setup_charger() UUGP case
$setupPattern = '(?s)#ifndef\s+SMALL_FLASH_DEVICE\s*\r?\n\s*case\s+ChargerType::UUGP:\s*\r?\n\s*charger\s*=\s*new\s+UUGPCharger\(\);\s*\r?\n\s*#endif\s*\r?\n\s*break;'

# First handle the current incorrect structure:
$incorrectSetupPattern = '(?s)\s*case\s+ChargerType::UUGP:\s*\r?\n\s*#ifndef\s+SMALL_FLASH_DEVICE\s*\r?\n\s*charger\s*=\s*new\s+UUGPCharger\(\);\s*\r?\n\s*#endif\s*\r?\n\s*break;'

$setupReplacement = @'
#ifndef SMALL_FLASH_DEVICE
    case ChargerType::UUGP:
      charger = new UUGPCharger();
      break;
#endif
'@

if ($content -match $incorrectSetupPattern) {
    $content = [regex]::Replace(
        $content,
        $incorrectSetupPattern,
        "`r`n$setupReplacement",
        1
    )
}
elseif ($content -notmatch $setupPattern) {
    throw "Kunne ikke finde UUGP-blokken i setup_charger() i $chargersCpp"
}

Write-Utf8NoBom $chargersCpp $content

Write-Host ""
Write-Host "UUGP SMALL_FLASH_DEVICE ændringer er anvendt." -ForegroundColor Green
Write-Host "Kør nu: git diff" -ForegroundColor Cyan
