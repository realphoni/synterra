$ErrorActionPreference = 'Stop'
$logPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'out/wsl-host-check.log'
Start-Transcript -Path $logPath -Force
try {
    Get-WindowsOptionalFeature -Online -FeatureName VirtualMachinePlatform | Select-Object FeatureName,State
    Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux | Select-Object FeatureName,State
    & "$env:SystemRoot\System32\bcdedit.exe" /enum
    Get-CimInstance Win32_ComputerSystem | Select-Object HypervisorPresent
} finally { Stop-Transcript }
