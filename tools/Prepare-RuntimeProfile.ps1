<#
.SYNOPSIS
Creates an isolated, verified Windows runtime profile for a candidate executable.
.DESCRIPTION
Copies the configured ROM, saves, mod configuration and installed mods without
changing the source profile. Assets and runtime DLLs are copied too: Windows
resolves assets relative to its working directory. Output must not already exist.
The manifest describes the pre-launch snapshot, not the active mod load order or
a claim that the supplied executable was built from the current source checkout.
Stop applications writing to the source profile before preparing a snapshot.
.EXAMPLE
./tools/Prepare-RuntimeProfile.ps1 -ProfileDirectory ./_working-directory/profile-seed `
  -Executable ./_working-directory/build-control/Zelda64Recompiled.exe `
  -AssetDirectory ./assets -OutputDirectory ./_working-directory/runtime-checks/control
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProfileDirectory,
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$AssetDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$RuntimeDirectory,
    # Additional mod-owned profile files/directories, relative to ProfileDirectory.
    [string[]]$AdditionalProfilePath = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-ExistingPath([string]$Path, [bool]$Directory) {
    $item = Get-Item -LiteralPath $Path -Force
    if ($item.PSIsContainer -ne $Directory) { throw "Unexpected path type: $Path" }
    return $item.FullName
}

$profileRoot = Get-ExistingPath $ProfileDirectory $true
$candidatePath = Get-ExistingPath $Executable $false
$assetRoot = Get-ExistingPath $AssetDirectory $true
if (!$RuntimeDirectory) { $RuntimeDirectory = Split-Path -Parent $candidatePath }
$runtimeRoot = Get-ExistingPath $RuntimeDirectory $true
$outputRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
if (Test-Path -LiteralPath $outputRoot) { throw "Output already exists; use a fresh directory: $outputRoot" }
foreach ($sourceRoot in @($profileRoot, $assetRoot, $runtimeRoot)) {
    $prefix = $sourceRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if ($outputRoot.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Output must be outside source directories: $sourceRoot"
    }
}

$entries = [Collections.Generic.List[object]]::new()
$destinations = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($reserved in @('manifest.json', 'portable.txt', 'PREPARATION-INCOMPLETE.txt')) {
    $null = $destinations.Add($reserved)
}
function Add-File([string]$Source, [string]$Relative, [string]$Category) {
    if (!$destinations.Add($Relative)) { throw "Duplicate destination: $Relative" }
    $file = Get-Item -LiteralPath $Source -Force
    if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw "File links are unsupported; provide a materialized source file: $Source"
    }
    $entries.Add([pscustomobject]@{
        source = $file.FullName; path = $Relative.Replace('\', '/'); category = $Category
        size = $file.Length; modified_utc = $file.LastWriteTimeUtc.ToString('o')
    })
}
function Add-Tree([string]$Source, [string]$Relative, [string]$Category) {
    # Enumerate manually so nested junctions never silently escape the seed.
    foreach ($child in Get-ChildItem -LiteralPath $Source -Force | Sort-Object Name) {
        if ($child.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Nested links are unsupported; materialize this source first: $($child.FullName)"
        }
        $childRelative = Join-Path $Relative $child.Name
        if ($child.PSIsContainer) { Add-Tree $child.FullName $childRelative $Category }
        else { Add-File $child.FullName $childRelative $Category }
    }
}

# These are the persisted core profile roots used by config.cpp and librecomp.
foreach ($name in @('controls.json', 'general.json', 'graphics.json', 'sound.json', 'mods.json')) {
    $path = Join-Path $profileRoot $name
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing profile configuration: $path" }
    Add-File $path $name 'configuration'
    foreach ($backup in Get-ChildItem -LiteralPath $profileRoot -File -Force | Where-Object { $_.Name.StartsWith($name + '.', [StringComparison]::OrdinalIgnoreCase) }) {
        Add-File $backup.FullName $backup.Name 'configuration-backup'
    }
}
$romName = 'mm.n64.us.1.0.z64'
Add-File (Join-Path $profileRoot $romName) $romName 'rom'
foreach ($name in @('saves', 'mod_config', 'mods')) {
    $path = Join-Path $profileRoot $name
    if (Test-Path -LiteralPath $path -PathType Container) { Add-Tree $path $name $name }
}
foreach ($name in $AdditionalProfilePath) {
    if ([IO.Path]::IsPathRooted($name)) { throw "Additional profile paths must be relative: $name" }
    $path = [IO.Path]::GetFullPath((Join-Path $profileRoot $name))
    if (!$path.StartsWith($profileRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Additional profile path escapes source: $name"
    }
    $name = $path.Substring($profileRoot.TrimEnd('\', '/').Length).TrimStart('\', '/')
    $item = Get-Item -LiteralPath $path -Force
    if ($item.PSIsContainer) { Add-Tree $path $name 'additional-profile' }
    else { Add-File $path $name 'additional-profile' }
}
Add-File $candidatePath 'Zelda64Recompiled.exe' 'executable'
foreach ($name in @('SDL2.dll', 'dxcompiler.dll', 'dxil.dll', 'recompcontrollerdb.txt')) {
    Add-File (Join-Path $runtimeRoot $name) $name 'runtime'
}
foreach ($dll in Get-ChildItem -LiteralPath $runtimeRoot -File -Filter '*.dll' | Sort-Object Name) {
    if (!$destinations.Contains($dll.Name)) { Add-File $dll.FullName $dll.Name 'runtime' }
}
Add-Tree $assetRoot 'assets' 'assets'

# Best-effort local-volume preflight; output uses full copies, not hard links.
# UNC paths, unavailable volume information and mount-point redirects may require
# the caller to check capacity separately. Copy failures still retain the marker.
$requiredBytes = [int64](($entries | Measure-Object -Property size -Sum).Sum) + 16MB
$outputVolumeRoot = [IO.Path]::GetPathRoot($outputRoot)
$availableBytes = $null
if ($outputVolumeRoot -match '^[A-Za-z]:[\\/]$') {
    try {
        $outputDrive = [IO.DriveInfo]::new($outputVolumeRoot)
        if ($outputDrive.IsReady -and $outputDrive.DriveType -ne [IO.DriveType]::Network) {
            $availableBytes = $outputDrive.AvailableFreeSpace
        }
    }
    catch {
        Write-Verbose "Could not query output free space: $($_.Exception.Message)"
    }
}
if ($null -ne $availableBytes -and $availableBytes -lt $requiredBytes) {
    throw "Insufficient disk space on ${outputVolumeRoot}: need $requiredBytes bytes (including 16 MiB cushion), available $availableBytes bytes. No output was created."
}

# Write a failure marker first. A profile is ready only when manifest.json exists.
$null = New-Item -ItemType Directory -Path $outputRoot
$marker = Join-Path $outputRoot 'PREPARATION-INCOMPLETE.txt'
Set-Content -LiteralPath $marker -Value 'Preparation has not completed. Do not launch this profile.' -Encoding UTF8
$manifestFiles = @(
    foreach ($entry in $entries) {
        $beforeHash = (Get-FileHash -LiteralPath $entry.source -Algorithm SHA256).Hash
        $destination = Join-Path $outputRoot $entry.path
        $null = New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force
        Copy-Item -LiteralPath $entry.source -Destination $destination
        $copiedHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
        if ($beforeHash -ne $copiedHash) { throw "Source changed while copying: $($entry.source)" }
        [ordered]@{ path = $entry.path; category = $entry.category; size = (Get-Item -LiteralPath $destination).Length; sha256 = $copiedHash; source = $entry.source }
    }
)
# Detect concurrent edits to enumerated source files before marking the run ready.
foreach ($entry in $entries) {
    $source = Get-Item -LiteralPath $entry.source -Force
    if ($source.Length -ne $entry.size -or $source.LastWriteTimeUtc.ToString('o') -ne $entry.modified_utc) {
        throw "Source profile/runtime changed during preparation: $($entry.source)"
    }
}
Set-Content -LiteralPath (Join-Path $outputRoot 'portable.txt') -Value 'Isolated validation profile. See manifest.json for the initial snapshot.' -Encoding UTF8
$manifest = [ordered]@{
    schema_version = 1
    prepared_utc = [DateTime]::UtcNow.ToString('o')
    source_profile = $profileRoot
    source_executable = $candidatePath
    working_directory = $outputRoot
    executable = (Join-Path $outputRoot 'Zelda64Recompiled.exe')
    note = 'Pre-launch snapshot. Does not attest executable source provenance or active mod load order. ROM and mod assets must remain local and ignored.'
    files = $manifestFiles
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot 'manifest.json') -Encoding UTF8
Remove-Item -LiteralPath $marker
[pscustomobject]@{ Executable = $manifest.executable; WorkingDirectory = $outputRoot; Manifest = (Join-Path $outputRoot 'manifest.json'); FileCount = $manifestFiles.Count }
