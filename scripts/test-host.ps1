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

$contentCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-DHELENGINE_N64_HAS_GENERATED_CORE=1',
    '-Itests/stubs', '-Isrc',
    'tests/Nintendo64ContentStreamSourceTests.cpp',
    'src/platform/n64/Nintendo64ContentStreamSource.cpp',
    '-o', 'build/n64-content-source-tests'
)
& docker @contentCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 content-source test compilation failed.' }
$contentRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-content-source-tests')
& docker @contentRun
if ($LASTEXITCODE -ne 0) { throw 'N64 content-source tests failed.' }

$lightCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/Nintendo64LightMathTests.cpp', 'src/platform/n64/Nintendo64LightMath.cpp',
    '-o', 'build/n64-light-math-tests'
)
& docker @lightCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 light-math test compilation failed.' }
$lightRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-light-math-tests')
& docker @lightRun
if ($LASTEXITCODE -ne 0) { throw 'N64 light-math tests failed.' }

$vertexCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-Itests/stubs', '-I.',
    'tests/Nintendo64VertexPackingTests.cpp',
    'src/platform/n64/Nintendo64VertexPacking.cpp',
    '-o', 'build/n64-vertex-packing-tests'
)
& docker @vertexCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 vertex-packing test compilation failed.' }
$vertexRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-vertex-packing-tests')
& docker @vertexRun
if ($LASTEXITCODE -ne 0) { throw 'N64 vertex-packing tests failed.' }
