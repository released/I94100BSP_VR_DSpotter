function Get-ReplyAudioState([string]$Root) {
    # Text needs a new WAV; changed WAV/UID inputs need Audio Tool encoding.
    $directory = Join-Path $Root 'AudioRes/ReplyPhrases'
    $profilePath = Join-Path $directory 'ReplyProfile.json'
    $profile = Read-VoiceJson $profilePath
    if (@($profile.phrases).Count -ne 23) { throw '目前韌體配置為 23 個音訊片段；新增回覆種類需先完成韌體播放配置。' }
    $source = Read-VoiceJson (Join-Path $directory 'WavSrc/PhraseSourceManifest.json')
    $seal = Read-VoiceJson (Join-Path $directory 'AudioBuildManifest.json')
    $profileHash = Get-VoiceHash ([IO.File]::ReadAllBytes($profilePath))
    $current = $true
    foreach ($property in $seal.sources.PSObject.Properties) {
        $path = Join-Path $Root $property.Name
        Assert-VoicePath $Root $path
        if (-not (Test-Path -LiteralPath $path) -or (Get-VoiceHash ([IO.File]::ReadAllBytes($path))) -ne $property.Value) { $current = $false }
    }
    $waves = @{}; $changedWave = $false; $textNeedsWave = $false; $sourceCurrent = $source.source_profile_sha256 -eq $profileHash
    $safeSynthesis = $source.voice -eq $profile.voice
    $latestInput = (Get-Item -LiteralPath (Join-Path $directory 'WavSrc/AirconReplies.uid')).LastWriteTimeUtc
    foreach ($phrase in $profile.phrases) {
        if ([IO.Path]::GetFileName($phrase.file) -ne $phrase.file) { throw 'WAV 檔名必須是專案內的單一檔名。' }
        $path = Join-Path $directory ('WavSrc/' + $phrase.file)
        [byte[]]$wave = [IO.File]::ReadAllBytes($path)
        if ([Text.Encoding]::ASCII.GetString($wave,0,4) -ne 'RIFF' -or [Text.Encoding]::ASCII.GetString($wave,8,4) -ne 'WAVE') { throw ('WAV 格式不符：' + $phrase.file) }
        $offset = 12; $formatOK = $false; $dataOK = $false
        while ($offset + 8 -le $wave.Length) {
            $tag = [Text.Encoding]::ASCII.GetString($wave,$offset,4)
            $size = [BitConverter]::ToUInt32($wave,$offset+4)
            if ($size -gt $wave.Length-$offset-8) { throw ('WAV 區段超出檔案：' + $phrase.file) }
            if ($tag -eq 'fmt ' -and $size -ge 16) {
                $formatOK = [BitConverter]::ToUInt16($wave,$offset+8) -eq 1 -and [BitConverter]::ToUInt16($wave,$offset+10) -eq 1 -and [BitConverter]::ToUInt32($wave,$offset+12) -eq 16000 -and [BitConverter]::ToUInt16($wave,$offset+22) -eq 16
            }
            if ($tag -eq 'data' -and $size -gt 0 -and ($size % 2) -eq 0) { $dataOK = $true }
            $offset += 8 + $size + ($size % 2)
        }
        if (-not $formatOK -or -not $dataOK) { throw ('請使用 16 kHz、16-bit PCM、單聲道 WAV：' + $phrase.file) }
        $hash = Get-VoiceHash $wave
        $waves[$phrase.file] = $hash
        $previous = @($source.entries | Where-Object file -eq $phrase.file) | Select-Object -First 1
        if ($null -eq $previous -or $previous.sha256 -ne $hash) { $sourceCurrent = $false; $safeSynthesis = $false }
        if ($previous.source_kind -eq 'recorded_or_imported') { $safeSynthesis = $false }
        if ($previous.spoken_text -ne $phrase.text -and $previous.sha256 -eq $hash) { $textNeedsWave = $true }
        $property = $seal.sources.PSObject.Properties['AudioRes/ReplyPhrases/WavSrc/' + $phrase.file]
        if ($null -eq $property -or $property.Value -ne $hash) { $changedWave = $true }
        $time = (Get-Item -LiteralPath $path).LastWriteTimeUtc
        if ($time -gt $latestInput) { $latestInput = $time }
    }
    $romPath = Join-Path $directory 'Output/AirconReplies_AudioInfoMerge.ROM'
    $romHash = ''
    if (Test-Path -LiteralPath $romPath) { $romHash = Get-VoiceHash ([IO.File]::ReadAllBytes($romPath)) }
    $oldRom = $seal.sources.PSObject.Properties['AudioRes/ReplyPhrases/Output/AirconReplies_AudioInfoMerge.ROM']
    $encodedReady = (Test-Path -LiteralPath $romPath) -and (Get-Item -LiteralPath $romPath).LastWriteTimeUtc -ge $latestInput
    if ($changedWave -and $oldRom.Value -eq $romHash) { $encodedReady = $false }
    if ($current -and $sourceCurrent) { $encodedReady = $true }
    [pscustomobject]@{ Current=($current -and $sourceCurrent); SourceCurrent=$sourceCurrent; Profile=$profile; Source=$source; Waves=$waves; ProfileHash=$profileHash; NeedsSynthesis=($safeSynthesis -and ($textNeedsWave -or $source.source_profile_sha256 -ne $profileHash)); TextNeedsWave=$textNeedsWave; EncodedReady=$encodedReady }
}

# Preserve imported recordings and seal only structurally current ROM output.
function Update-ReplyAudioMetadata([string]$Root) {
    $state = Get-ReplyAudioState $Root
    if ($state.Current) { return }
    if ($state.TextNeedsWave) { throw '回覆文字已變更，但 WAV 尚未更新；請先產生或替換對應 WAV。' }
    if (-not $state.EncodedReady) { throw '回覆音訊尚未完成編碼；請在 Audio Tool 按 F7，確認 0 errors 後再執行一鍵產生。' }
    if (-not $state.SourceCurrent) {
        $entries = @($state.Profile.phrases | Sort-Object index | ForEach-Object {
            $phrase = $_; $path = Join-Path $Root ('AudioRes/ReplyPhrases/WavSrc/' + $phrase.file)
            [ordered]@{ index=$phrase.index; file=$phrase.file; role=$phrase.role; command=$phrase.command; map_ids=@($phrase.map_ids); spoken_text=$phrase.text; bytes=(Get-Item -LiteralPath $path).Length; sha256=$state.Waves[$phrase.file]; source_kind='recorded_or_imported' }
        })
        $source = [ordered]@{ schema_version=1; release='V005'; sample_rate=16000; bits_per_sample=16; channels=1; voice='mixed_or_imported'; rate=0; nuone_bits_per_sample=1.0; prefix_gap_ms=$state.Profile.prefix_gap_ms; source_profile='AudioRes/ReplyPhrases/ReplyProfile.json'; source_profile_sha256=$state.ProfileHash; entries=$entries }
        [IO.File]::WriteAllBytes((Join-Path $Root 'AudioRes/ReplyPhrases/WavSrc/PhraseSourceManifest.json'), (Get-VoiceUtf8 (ConvertTo-VoiceJson $source) $true))
    }
    & (Join-Path $Root 'AudioRes/ReplyPhrases/SealAudioBuild.ps1')
}

function Open-ReplyAudioTool([string]$Root) {
    $project = Join-Path $Root 'AudioRes/ReplyPhrases/AirconReplies.wba'
    $base = Join-Path $env:SystemDrive 'Nuvoton/NSPPlayListEditor'
    $tool = $null
    if (Test-Path -LiteralPath $base) {
        foreach ($version in (Get-ChildItem -LiteralPath $base -Directory | Sort-Object Name -Descending)) {
            $candidate = Join-Path $version.FullName 'NSP_SDS/SDS_NSP/AudioTool/AudioTool.exe'
            if (Test-Path -LiteralPath $candidate) { $tool = $candidate; break }
        }
    }
    try {
        if ($tool) { [void](Start-Process -FilePath $tool -ArgumentList ('"'+$project+'"') -WindowStyle Normal) }
        else { [void](Start-Process -FilePath $project -WindowStyle Normal) }
    }
    catch { Write-Host ('請手動開啟 Audio Tool 工程：' + $project) }
}
