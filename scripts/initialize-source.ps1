$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$lock = Get-Content -LiteralPath (Join-Path $root 'dependencies.lock.json') -Raw | ConvertFrom-Json
foreach ($dependency in @($lock.submodules | Where-Object windowsBuild)) {
    $target = [IO.Path]::GetFullPath((Join-Path $root $dependency.path))
    if (-not $target.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid dependency path.' }
    if ($dependency.url -notlike 'https://github.com/*') { throw 'Unexpected dependency host.' }
    $created = $false
    if (-not (Test-Path -LiteralPath (Join-Path $target '.git'))) {
        New-Item -ItemType Directory -Path $target -Force | Out-Null
        & git -C $target init
        & git -C $target remote add origin $dependency.url
        if ($LASTEXITCODE) { throw "Cannot initialize $($dependency.path)" }
        $created = $true
    }
    $current = if ($created) { '' } else { & git -C $target rev-parse HEAD }
    if ($current -ne $dependency.commit) {
        & git -C $target fetch --depth 1 origin $dependency.commit
        if ($LASTEXITCODE) { throw "Cannot fetch $($dependency.path)" }
        & git -C $target checkout --detach $dependency.commit
        if ($LASTEXITCODE) { throw "Cannot check out $($dependency.path)" }
    }
    & git -C $target submodule update --init --recursive
    if ($LASTEXITCODE) { throw "Cannot initialize nested dependencies for $($dependency.path)" }
}
