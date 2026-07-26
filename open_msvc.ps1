$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $Root
try {
    & "$Root\generate_msvc.ps1"
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC project generation failed with exit code $LASTEXITCODE."
    }

    $Selected = (Get-Content "$Root\build\.active_generator" -Raw).Trim()
    $Solution = "$Root\build\$Selected\epoch_integrated_opengl_scene.sln"
    if (-not (Test-Path $Solution)) {
        throw "Generated solution was not found: $Solution"
    }
    Start-Process $Solution
}
finally {
    Pop-Location
}
