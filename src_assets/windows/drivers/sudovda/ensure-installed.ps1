$ErrorActionPreference = 'Stop'
$existing = @(Get-PnpDevice -Class Display -PresentOnly -ErrorAction Stop | Where-Object {
    if ($_.Status -ne 'OK') { return $false }
    $ids = (Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName 'DEVPKEY_Device_HardwareIds' -ErrorAction SilentlyContinue).Data
    return @($ids | Where-Object { $_ -ieq 'Root\SudoMaker\SudoVDA' -or $_ -ieq 'SudoVDA' }).Count -gt 0
})
if ($existing.Count) {
    Write-Output 'A working SudoVDA driver is already present; keeping it.'
    exit 0
}
foreach ($name in @('nefconc.exe','SudoVDA.dll','SudoVDA.inf','sudovda.cat','sudovda.cer')) {
    if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name))) { throw "Missing driver package file: $name" }
}
& (Join-Path $PSScriptRoot 'install.bat')
if ($LASTEXITCODE) { throw 'SudoVDA installation failed.' }
