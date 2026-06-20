param(
    [string]$NdkPath = $env:ANDROID_NDK_HOME,
    [string]$ZygiskHeaderRevision = 'master',
    [string]$ZygiskHeaderSha256 = ''
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($NdkPath)) {
    $NdkPath = $env:ANDROID_NDK_ROOT
}
if ([string]::IsNullOrWhiteSpace($NdkPath)) {
    throw 'Set ANDROID_NDK_HOME/ANDROID_NDK_ROOT or pass -NdkPath.'
}

$NdkBuild = Join-Path $NdkPath 'ndk-build.cmd'
if (-not (Test-Path $NdkBuild)) {
    throw "ndk-build.cmd was not found under $NdkPath"
}

$Header = Join-Path $Root 'module\jni\zygisk.hpp'
$HeaderValid = $false
if (Test-Path $Header) {
    $HeaderText = Get-Content $Header -Raw
    $HeaderValid = $HeaderText.Contains('#define ZYGISK_API_VERSION 4') -and
        $HeaderText.Contains('REGISTER_ZYGISK_MODULE') -and
        $HeaderText.Contains('REGISTER_ZYGISK_COMPANION')
}
if (-not $HeaderValid) {
    $Url = "https://raw.githubusercontent.com/topjohnwu/zygisk-module-sample/$ZygiskHeaderRevision/module/jni/zygisk.hpp"
    $TempHeader = "$Header.tmp"
    Remove-Item $TempHeader -Force -ErrorAction SilentlyContinue
    Invoke-WebRequest -Uri $Url -OutFile $TempHeader
    $HeaderText = Get-Content $TempHeader -Raw
    if (-not ($HeaderText.Contains('#define ZYGISK_API_VERSION 4') -and
              $HeaderText.Contains('REGISTER_ZYGISK_MODULE') -and
              $HeaderText.Contains('REGISTER_ZYGISK_COMPANION'))) {
        Remove-Item $TempHeader -Force -ErrorAction SilentlyContinue
        throw 'Downloaded file is not the expected Zygisk API v4 header.'
    }
    if (-not [string]::IsNullOrWhiteSpace($ZygiskHeaderSha256)) {
        $ActualHash = (Get-FileHash $TempHeader -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($ActualHash -ne $ZygiskHeaderSha256.ToLowerInvariant()) {
            Remove-Item $TempHeader -Force -ErrorAction SilentlyContinue
            throw "zygisk.hpp SHA-256 mismatch: $ActualHash"
        }
    }
    Move-Item $TempHeader $Header -Force
}

$VersionLine = Get-Content (Join-Path $Root 'module\module.prop') |
    Where-Object { $_ -like 'version=*' } | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($VersionLine)) {
    throw 'Could not read module version.'
}
$Version = $VersionLine.Substring('version='.Length)
$Out = Join-Path $Root 'out'
$Obj = Join-Path $Out 'obj'
$Libs = Join-Path $Out 'libs'
$Package = Join-Path $Out 'package'
$Dist = Join-Path $Root 'dist'

Remove-Item $Obj, $Libs, $Package -Recurse -Force -ErrorAction SilentlyContinue
New-Item (Join-Path $Package 'zygisk') -ItemType Directory -Force | Out-Null
New-Item $Dist -ItemType Directory -Force | Out-Null

& $NdkBuild `
    "NDK_PROJECT_PATH=$Root\module" `
    "APP_BUILD_SCRIPT=$Root\module\jni\Android.mk" `
    "NDK_APPLICATION_MK=$Root\module\jni\Application.mk" `
    "NDK_OUT=$Obj" `
    "NDK_LIBS_OUT=$Libs"
if ($LASTEXITCODE -ne 0) { throw 'NDK build failed' }

foreach ($Abi in @('arm64-v8a', 'armeabi-v7a', 'x86_64')) {
    $Source = Join-Path $Libs "$Abi\libzygisk_secure_capture.so"
    if (-not (Test-Path $Source)) { throw "Missing built library: $Source" }
    Copy-Item $Source (Join-Path $Package "zygisk\$Abi.so")
}
Copy-Item (Join-Path $Root 'module\module.prop') $Package
Copy-Item (Join-Path $Root 'module\customize.sh') $Package
Copy-Item (Join-Path $Root 'module\post-fs-data.sh') $Package
Copy-Item (Join-Path $Root 'module\service.sh') $Package
Copy-Item (Join-Path $Root 'module\config') $Package -Recurse
Copy-Item (Join-Path $Root 'LICENSE') $Package

$Zip = Join-Path $Dist "Zygisk-Secure-Capture-$Version.zip"
Remove-Item $Zip -Force -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $Package '*') -DestinationPath $Zip -CompressionLevel Optimal
Write-Host "Built module: $Zip"
