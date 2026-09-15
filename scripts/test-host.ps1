$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
New-Item -ItemType Directory -Force -Path (Join-Path $root 'build') | Out-Null
$mount = "${root}:/workspace"
$compile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/FrameClockTests.cpp', 'src/platform/n64/FrameClock.cpp',
    '-o', 'build/frame-clock-tests'
)
& docker @compile
if ($LASTEXITCODE -ne 0) { throw 'Frame clock compilation failed.' }
$run = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/frame-clock-tests')
& docker @run
if ($LASTEXITCODE -ne 0) { throw 'Frame clock tests failed.' }
