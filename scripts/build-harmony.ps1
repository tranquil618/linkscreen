param(
    [string]$DevEcoHome = 'D:\DevEco Studio'
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$sourceProject = Join-Path $repositoryRoot 'apps\harmony-client'
$sourceProtocol = Join-Path $repositoryRoot 'native\protocol'
$hvigor = Join-Path $DevEcoHome 'tools\hvigor\bin\hvigorw.bat'
$sdkHome = Join-Path $DevEcoHome 'sdk'
$javaHome = Join-Path $DevEcoHome 'jbr'

foreach ($requiredPath in @($sourceProject, $sourceProtocol, $hvigor, $sdkHome, $javaHome)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required path does not exist: $requiredPath"
    }
}

$stageRoot = Join-Path $env:TEMP ("linkscreen-harmony-" + [guid]::NewGuid().ToString('N'))
$stageProject = Join-Path $stageRoot 'apps\harmony-client'
$stageProtocol = Join-Path $stageRoot 'native\protocol'
$artifactDirectory = Join-Path $repositoryRoot 'artifacts\harmony'

try {
    New-Item -ItemType Directory -Path $stageProject -Force | Out-Null
    New-Item -ItemType Directory -Path $stageProtocol -Force | Out-Null
    Copy-Item -Path (Join-Path $sourceProject '*') -Destination $stageProject -Recurse -Force
    Copy-Item -Path (Join-Path $sourceProtocol '*') -Destination $stageProtocol -Recurse -Force

    $env:DEVECO_SDK_HOME = $sdkHome
    $env:JAVA_HOME = $javaHome
    $env:Path = "$javaHome\bin;$env:Path"

    Push-Location $stageProject
    try {
        & $hvigor assembleHap `
            --mode module `
            -p product=default `
            -p module=entry@default `
            -p buildMode=debug `
            --no-daemon
        if ($LASTEXITCODE -ne 0) {
            throw "Hvigor failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }

    $hap = Get-ChildItem -LiteralPath (Join-Path $stageProject 'entry\build') `
        -Recurse -Filter 'entry-default-unsigned.hap' | Select-Object -First 1
    if ($null -eq $hap) {
        throw 'Hvigor succeeded but the unsigned HAP was not found.'
    }

    New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null
    $artifactPath = Join-Path $artifactDirectory 'linkscreen-entry-default-unsigned.hap'
    Copy-Item -LiteralPath $hap.FullName -Destination $artifactPath -Force
    Write-Host "HarmonyOS HAP created: $artifactPath"
} finally {
    $tempRoot = [System.IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
    $resolvedStage = [System.IO.Path]::GetFullPath($stageRoot)
    if ($resolvedStage.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) `
        -and (Split-Path -Leaf $resolvedStage).StartsWith('linkscreen-harmony-')) {
        Remove-Item -LiteralPath $resolvedStage -Recurse -Force -ErrorAction SilentlyContinue
    }
}

