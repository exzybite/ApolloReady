param([string]$SevenZip = '7z.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$download = Join-Path $root 'build\downloads'
$destination = Join-Path $root 'src_assets\windows\drivers\sudovda'
$expectedFiles = @{
    'nefconc.exe'='9DBA1F1A9E2B21843C4A0CA6C6FFA9E747250DD6E42C8C14325CA069AD43EA8F'
    'SudoVDA.dll'='47EE263CB5DE9382C6630A2D7F3DAFEC4A49419F953BEEC869CA5DD0C460FF63'
}
$complete = $true
foreach ($name in $expectedFiles.Keys) {
    $file = Join-Path $destination $name
    if (-not (Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file).Hash -ne $expectedFiles[$name]) { $complete = $false }
}
if ($complete) { Write-Output 'Driver payload already verified.'; exit 0 }
New-Item -ItemType Directory -Path $download -Force | Out-Null
$archive = Join-Path $download 'Apollo-0.4.6.exe'
if (-not (Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest -Uri 'https://github.com/ClassicOldSong/Apollo/releases/download/v0.4.6/Apollo-0.4.6.exe' -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive).Hash -ne '42B2AEFAACB3474511517A56B96EE9F0517F30AC38B5DD2FDA9FD5B478F5021A') { throw 'Official prerequisite archive SHA256 mismatch.' }
$extracted = Join-Path $download 'driver-payload'
& $SevenZip x $archive "-o$extracted" '-y' 'drivers/sudovda/nefconc.exe' 'drivers/sudovda/SudoVDA.dll'
if ($LASTEXITCODE) { throw 'Driver payload extraction failed.' }
foreach ($name in $expectedFiles.Keys) {
    $file = Join-Path $extracted "drivers\sudovda\$name"
    if (-not (Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file).Hash -ne $expectedFiles[$name]) { throw "Driver file SHA256 mismatch: $name" }
    Copy-Item -LiteralPath $file -Destination $destination -Force
}
# The official archive is read as data only; Apollo is never executed or installed.
Write-Output 'Pinned driver prerequisites extracted and verified.'
