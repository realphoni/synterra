$ErrorActionPreference = 'Stop'
$outputFolder = Join-Path (Split-Path -Parent $PSScriptRoot) 'out/vmware-mode'
New-Item -ItemType Directory -Force -Path $outputFolder | Out-Null
$resultFile = Join-Path $outputFolder 'result.json'
$result = [ordered]@{ Success = $false; RestartRequired = $false; Backup = $null; Error = $null }
$exitCode = 0
$transcriptStarted = $false
try {
    $principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run this script as administrator to change the Windows hypervisor boot setting.'
    }
    Start-Transcript -Path (Join-Path $outputFolder 'change.log') -Force | Out-Null
    $transcriptStarted = $true
    $bcdTool = Join-Path $env:SystemRoot 'System32/bcdedit.exe'
    $before = & $bcdTool /enum 2>&1
    if ($LASTEXITCODE -ne 0) { throw 'Could not read the boot configuration; no changes made.' }
    $before | Set-Content -LiteralPath (Join-Path $outputFolder 'boot-before.txt')
    $backup = Join-Path $outputFolder ('boot-before-vmware-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.bcd')
    & $bcdTool /export $backup
    if ($LASTEXITCODE -ne 0) { throw 'Boot configuration backup failed; no changes made.' }
    $result.Backup = $backup
    & $bcdTool /set hypervisorlaunchtype Off
    if ($LASTEXITCODE -ne 0) { throw 'Hypervisor boot setting update failed.' }
    $after = & $bcdTool /enum 2>&1
    if ($LASTEXITCODE -ne 0) { throw 'Could not verify the boot configuration.' }
    $after | Set-Content -LiteralPath (Join-Path $outputFolder 'boot-after.txt')
    if (($after | Out-String) -notmatch '(?im)^\s*hypervisorlaunchtype\s+Off\s*$') {
        throw 'The boot configuration did not report hypervisorlaunchtype Off; inspect boot-after.txt.'
    }
    $result.Success = $true
    $result.RestartRequired = $true
    Write-Output 'Windows hypervisor launch is disabled. Restart Windows to activate VMware mode.'
} catch {
    $result.Error = $_.Exception.Message
    Write-Output $result.Error
    $exitCode = 1
} finally {
    $result | ConvertTo-Json | Set-Content -LiteralPath $resultFile
    if ($transcriptStarted) { Stop-Transcript | Out-Null }
}
exit $exitCode
