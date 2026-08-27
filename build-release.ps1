$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$juceCmake = Join-Path $projectRoot 'vendor\JUCE\CMakeLists.txt'

function Assert-CommandSucceeded([string]$step) {
    if ($LASTEXITCODE -ne 0) {
        throw "$step failed with exit code $LASTEXITCODE. Distribution files were not copied."
    }
}

if (-not (Test-Path -LiteralPath $cmake)) {
    throw 'Visual Studio 2022 CMake was not found.'
}

if (-not (Test-Path -LiteralPath $juceCmake)) {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if ($null -eq $git -or -not (Test-Path -LiteralPath (Join-Path $projectRoot '.git'))) {
        throw 'JUCE is missing. Clone with --recurse-submodules, then run this script again.'
    }
    & $git.Source -C $projectRoot submodule update --init --depth 1
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $juceCmake)) {
        throw 'The JUCE submodule could not be initialized.'
    }
}

& $cmake -S $projectRoot -B "$projectRoot\build" -G 'Visual Studio 17 2022' -A x64
Assert-CommandSucceeded 'CMake configure'
& $cmake --build "$projectRoot\build" --config Release --target VOIDWORM_Standalone VOIDWORM_VST3 --parallel
Assert-CommandSucceeded 'Release build'

$vstSource = "$projectRoot\build\VOIDWORM_artefacts\Release\VST3\VOIDWORM.vst3"
$standaloneSource = "$projectRoot\build\VOIDWORM_artefacts\Release\Standalone\VOIDWORM.exe"
$dist = "$projectRoot\dist"

if (-not (Test-Path -LiteralPath $vstSource) -or -not (Test-Path -LiteralPath $standaloneSource)) {
    throw 'Release artifacts were not found. Distribution files were not copied.'
}

New-Item -ItemType Directory -Force -Path $dist | Out-Null
New-Item -ItemType Directory -Force -Path "$dist\VOIDWORM.vst3" | Out-Null
Copy-Item -Path "$vstSource\*" -Destination "$dist\VOIDWORM.vst3" -Recurse -Force
Copy-Item -LiteralPath $standaloneSource -Destination "$dist\VOIDWORM.exe" -Force

Write-Host "Built: $dist\VOIDWORM.vst3"
Write-Host "Built: $dist\VOIDWORM.exe"
