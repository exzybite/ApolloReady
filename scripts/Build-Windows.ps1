param([string]$MsysRoot = 'C:\msys64', [switch]$InstallDependencies)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) { throw 'Install MSYS2 first or specify -MsysRoot.' }
& (Join-Path $PSScriptRoot 'initialize-source.ps1')
$previous = $env:MSYSTEM
try {
    $env:MSYSTEM = 'UCRT64'
    if ($InstallDependencies) {
        & $bash --login (Join-Path $PSScriptRoot 'install-build-deps.sh').Replace('\','/')
        if ($LASTEXITCODE) { throw 'Build dependency installation failed.' }
    }
    & (Join-Path $PSScriptRoot 'prepare-windows-prerequisites.ps1') -SevenZip (Join-Path $MsysRoot 'ucrt64\bin\7z.exe')
    & $bash --login (Join-Path $PSScriptRoot 'build-windows.sh').Replace('\','/')
    if ($LASTEXITCODE) { throw 'Build or packaging failed.' }
} finally { $env:MSYSTEM = $previous }
Write-Output "Installer: $root\build-gameready\cpack_artifacts"
