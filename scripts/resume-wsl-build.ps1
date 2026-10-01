$ErrorActionPreference = 'Stop'
$workspacePath = Split-Path -Parent $PSScriptRoot
$archImagePath = Join-Path $workspacePath 'out/installers/archlinux-2026.10.01.179549.wsl'
$distributionName = 'SynterraBuild'

$registeredNames = (& wsl.exe --list --quiet 2>&1 | Out-String).Replace([string][char]0, '').Trim() -split '\r?\n'
if ($LASTEXITCODE -ne 0) {
    throw 'WSL is not ready. Restart Windows to apply the installed components, then retry.'
}
if ($registeredNames.Trim() -notcontains $distributionName) {
    if (-not (Test-Path -LiteralPath $archImagePath)) {
        throw "Downloaded Arch WSL image missing: $archImagePath"
    }
    & wsl.exe --install --from-file $archImagePath --name $distributionName --no-launch
    if ($LASTEXITCODE -ne 0) { throw 'Arch WSL registration failed.' }
}

& wsl.exe --distribution $distributionName --user root -- bash /mnt/e/tuff/scripts/build-wsl.sh /mnt/e/tuff /mnt/e/tuff/out
if ($LASTEXITCODE -ne 0) { throw 'Synterra ISO build failed; inspect the build log.' }
Write-Output "ISO output: $workspacePath\out"
