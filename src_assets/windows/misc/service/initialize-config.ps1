$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$directory = Join-Path $root 'config'
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$file = Join-Path $directory 'sunshine.conf'
if (-not (Test-Path -LiteralPath $file)) {
    # No port override: use the standard GameStream ports on a fresh install.
    'sunshine_name = ApolloReady' | Set-Content -LiteralPath $file -Encoding ASCII
}
