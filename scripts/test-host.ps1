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

$texelCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/Nintendo64TexelPackingTests.cpp', 'src/platform/n64/Nintendo64TexelPacking.cpp',
    '-o', 'build/n64-texel-packing-tests'
)
& docker @texelCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 texel-packing test compilation failed.' }
$texelRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-texel-packing-tests')
& docker @texelRun
if ($LASTEXITCODE -ne 0) { throw 'N64 texel-packing tests failed.' }

$roundedRectCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/Nintendo64RoundedRectGeometryTests.cpp', 'src/platform/n64/Nintendo64RoundedRectGeometry.cpp',
    '-o', 'build/n64-rounded-rect-geometry-tests'
)
& docker @roundedRectCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 rounded-rect-geometry test compilation failed.' }
$roundedRectRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-rounded-rect-geometry-tests')
& docker @roundedRectRun
if ($LASTEXITCODE -ne 0) { throw 'N64 rounded-rect-geometry tests failed.' }

$quadBlitCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/Nintendo64QuadBlitGeometryTests.cpp', 'src/platform/n64/Nintendo64QuadBlitGeometry.cpp',
    '-o', 'build/n64-quad-blit-geometry-tests'
)
& docker @quadBlitCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 quad-blit-geometry test compilation failed.' }
$quadBlitRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-quad-blit-geometry-tests')
& docker @quadBlitRun
if ($LASTEXITCODE -ne 0) { throw 'N64 quad-blit-geometry tests failed.' }

$clipRectCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/Nintendo64ClipRectStackTests.cpp', 'src/platform/n64/Nintendo64ClipRectStack.cpp',
    '-o', 'build/n64-clip-rect-stack-tests'
)
& docker @clipRectCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 clip-rect-stack test compilation failed.' }
$clipRectRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-clip-rect-stack-tests')
& docker @clipRectRun
if ($LASTEXITCODE -ne 0) { throw 'N64 clip-rect-stack tests failed.' }

$cpuProfileCompile = @(
    'run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', 'g++',
    '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I.', 'tests/Nintendo64CpuProfileWindowTests.cpp', 'src/platform/n64/Nintendo64CpuProfileWindow.cpp',
    'src/platform/n64/Nintendo64CpuProfileTypeName.cpp',
    '-o', 'build/n64-cpu-profile-window-tests'
)
& docker @cpuProfileCompile
if ($LASTEXITCODE -ne 0) { throw 'N64 CPU profile window test compilation failed.' }
$cpuProfileRun = @('run', '--rm', '-v', $mount, '-w', '/workspace',
    'helengine-n64-build', './build/n64-cpu-profile-window-tests')
& docker @cpuProfileRun
if ($LASTEXITCODE -ne 0) { throw 'N64 CPU profile window tests failed.' }

$buildRoot = (Resolve-Path (Join-Path $root 'build')).Path
$probeName = 'n64-make-clean-probe-' + [Guid]::NewGuid().ToString('N')
$probeRoot = [IO.Path]::GetFullPath((Join-Path $buildRoot $probeName))
$buildPrefix = $buildRoot.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $probeRoot.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Make clean probe escaped the build directory: $probeRoot"
}
$probeWorkspace = Join-Path $probeRoot 'workspace'
$probeCache = Join-Path $probeRoot 'cache'
New-Item -ItemType Directory -Force -Path $probeWorkspace, $probeCache | Out-Null
try {
    Copy-Item -LiteralPath (Join-Path $root 'Makefile') -Destination (Join-Path $probeWorkspace 'Makefile')
    Set-Content -LiteralPath (Join-Path $probeWorkspace 'helengine_n64.z64') -Value 'probe' -NoNewline
    Set-Content -LiteralPath (Join-Path $probeCache 'sentinel.o') -Value 'probe' -NoNewline
    $cleanCache = @(
        'run', '--rm',
        '-v', "${probeWorkspace}:/workspace",
        '-v', "${probeCache}:/n64-native-cache",
        '-w', '/workspace', 'helengine-n64-build', 'make',
        'BUILD_DIR=/n64-native-cache', 'clean'
    )
    & docker @cleanCache
    if ($LASTEXITCODE -ne 0) { throw 'N64 make clean failed for a mounted native object cache.' }
    if (-not (Test-Path -LiteralPath $probeCache)) { throw 'N64 make clean removed the native cache mount point.' }
    if (Test-Path -LiteralPath (Join-Path $probeCache 'sentinel.o')) { throw 'N64 make clean left a native cache object behind.' }
    if (Test-Path -LiteralPath (Join-Path $probeWorkspace 'helengine_n64.z64')) { throw 'N64 make clean left the ROM behind.' }
} finally {
    if (Test-Path -LiteralPath $probeRoot) { Remove-Item -LiteralPath $probeRoot -Recurse -Force }
}
