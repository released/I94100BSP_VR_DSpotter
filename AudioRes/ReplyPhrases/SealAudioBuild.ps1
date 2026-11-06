$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$profilePath = Join-Path $PSScriptRoot 'ReplyProfile.json'
$sourcePath = Join-Path $PSScriptRoot 'WavSrc\PhraseSourceManifest.json'
$romPath = Join-Path $PSScriptRoot 'Output\AirconReplies_AudioInfoMerge.ROM'
$profile = Get-Content -LiteralPath $profilePath -Raw -Encoding UTF8 | ConvertFrom-Json
$source = Get-Content -LiteralPath $sourcePath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($source.source_profile_sha256 -ne (Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash) {
    throw 'Reply profile changed; regenerate phrase WAVs and rebuild the NuOne project.'
}
$raw = [IO.File]::ReadAllBytes($romPath)
if ([BitConverter]::ToUInt32($raw,0) -ne 23 -or $raw.Length+16 -gt 65536) {
    throw 'Expected 23 clips within the internal audio region.'
}
$inputs = @($profilePath,$sourcePath,(Join-Path $PSScriptRoot 'WavSrc\AirconReplies.uid'))
foreach ($entry in $source.entries) {
    $path = Join-Path $PSScriptRoot ('WavSrc\'+$entry.file)
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.sha256) {
        throw ('Source WAV changed: '+$entry.file)
    }
    $inputs += $path
}
$romTime = (Get-Item -LiteralPath $romPath).LastWriteTimeUtc
foreach ($path in $inputs) {
    if ((Get-Item -LiteralPath $path).LastWriteTimeUtc -gt $romTime) {
        throw ('Source is newer than the encoded ROM: '+$path)
    }
}
$inputs += (Join-Path $PSScriptRoot 'Output\AirconReplies_AudioInfo.h')
$inputs += $romPath
$hashes = [ordered]@{}
foreach ($path in $inputs) {
    $relative = [IO.Path]::GetFullPath($path).Substring($root.Length+1).Replace('\','/')
    $hashes[$relative] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}
$record = [ordered]@{
    schema_version=1;codec='NuOne';bits_per_sample=1.0;sample_rate=16000;clips=23
    rom='AudioRes/ReplyPhrases/Output/AirconReplies_AudioInfoMerge.ROM'
    sources=$hashes
}
$output = Join-Path $PSScriptRoot 'AudioBuildManifest.json'
[IO.File]::WriteAllText($output,($record|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding -ArgumentList $true))
Write-Output ('SUCCESS: verified 23 NuOne 1.0 source clips; ROM '+$raw.Length+' bytes')
Write-Output $output
