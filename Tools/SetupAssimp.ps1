param(
	[string]$CMake = '',
	[string]$Generator = 'Visual Studio 18 2026'
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $CMake) {
	$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
	if (Test-Path -LiteralPath $vswhere) {
		$installation = & $vswhere -latest -prerelease -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
		if ($installation) {
			$bundledCMake = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
			if (Test-Path -LiteralPath $bundledCMake) { $CMake = $bundledCMake }
		}
	}
	if (-not $CMake) { $CMake = 'cmake' }
}
$dependencyRoot = Join-Path $repoRoot 'Libraries/Assimp'
$archivePath = Join-Path $dependencyRoot 'assimp-6.0.2.tar.gz'
$expectedHash = 'D1822D9A19C9205D6E8BC533BF897174DDB360CE504680F294170CC1D6319751'
New-Item -ItemType Directory -Force $dependencyRoot | Out-Null
if (-not (Test-Path -LiteralPath $archivePath)) {
	Invoke-WebRequest -Uri 'https://github.com/assimp/assimp/archive/refs/tags/v6.0.2.tar.gz' -OutFile $archivePath
}
if ((Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash -ne $expectedHash) {
	throw 'Assimp archive checksum mismatch. Remove the archive and download it again.'
}
$sourceRoot = Join-Path $dependencyRoot 'assimp-6.0.2'
if (-not (Test-Path -LiteralPath $sourceRoot)) {
	& tar -xzf $archivePath -C $dependencyRoot
	if ($LASTEXITCODE -ne 0) { throw 'Assimp extraction failed.' }
}
$buildRoot = Join-Path $dependencyRoot 'build'
$installRoot = Join-Path $dependencyRoot 'install'
& $CMake -S $sourceRoot -B $buildRoot -G $Generator -A x64 "-DCMAKE_INSTALL_PREFIX=$installRoot" `
	-DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF `
	-DASSIMP_BUILD_SAMPLES=OFF -DASSIMP_BUILD_ZLIB=ON -DASSIMP_NO_EXPORT=ON -DASSIMP_INSTALL_PDB=OFF `
	-DASSIMP_WARNINGS_AS_ERRORS=OFF '-DLIBRARY_SUFFIX=' '-DCMAKE_CXX_FLAGS=/EHsc' `
	'-DCMAKE_CXX_FLAGS_DEBUG=/Zi /Od /RTC1' '-DCMAKE_C_FLAGS_DEBUG=/Zi /Od /RTC1' `
	'-DCMAKE_CXX_FLAGS_RELEASE=/O2 /DNDEBUG' '-DCMAKE_C_FLAGS_RELEASE=/O2 /DNDEBUG' `
	'-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL'
if ($LASTEXITCODE -ne 0) { throw 'Assimp configuration failed. Use a CMake version supporting your Visual Studio generator.' }
foreach ($configuration in @('Debug', 'Release')) {
	& $CMake --build $buildRoot --config $configuration --target INSTALL --parallel 6
	if ($LASTEXITCODE -ne 0) { throw "Assimp $configuration build failed." }
}
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination (Join-Path $installRoot 'LICENSE')
Write-Output 'Assimp 6.0.2 installed. Build VulkanProj.vcxproj for x64.'
