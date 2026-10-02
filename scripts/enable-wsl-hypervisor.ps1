$ErrorActionPreference = 'Stop'
$outputFolder = Join-Path (Split-Path -Parent $PSScriptRoot) 'out'
$bcdTool = "$env:SystemRoot\System32\bcdedit.exe"
Start-Transcript -Path (Join-Path $outputFolder 'wsl-hypervisor-fix.log') -Force
try {
    $backupFile = Join-Path $outputFolder ('boot-config-before-wsl-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.bcd')
    & $bcdTool /export $backupFile
    if ($LASTEXITCODE -ne 0) { throw 'Boot configuration backup failed; no changes made.' }
    & $bcdTool /set hypervisorlaunchtype Auto
    if ($LASTEXITCODE -ne 0) { throw 'Hypervisor setting update failed.' }
    & $bcdTool /enum
    Write-Output "Backup saved: $backupFile"
    Write-Output 'Restart Windows to activate the hypervisor. No reboot was initiated.'
} finally { Stop-Transcript }
