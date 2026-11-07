$cliArgs = @($args)
$ErrorActionPreference = 'Stop'
<#
Normal entry: BuildLatestModel.cmd. Prepare INI imports, wait for DSMT/Audio
Tool when needed, then build in an owned staging directory. Publish only
after source snapshot checks pass. --no-pause never launches GUI tools or
synthesizes new WAV files.
#>
[Console]::OutputEncoding = New-Object Text.UTF8Encoding -ArgumentList $false
$OutputEncoding = [Console]::OutputEncoding
. (Join-Path $PSScriptRoot 'VoiceMaintenance.ps1')
. (Join-Path $PSScriptRoot 'ReplyAudioBuild.ps1')
. (Join-Path $PSScriptRoot 'VoiceIni.ps1')
$root = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$pause = $true; $stage = $null; $lock = $null; $log = New-Object Text.StringBuilder; $exitCode = 1
foreach ($argument in $cliArgs) {
    if ($argument -in @('--no-pause','-NoPause')) { $pause = $false }
    elseif ($argument -eq '--pause') { $pause = $true }
    elseif ($argument -in @('--help','-h')) { Write-Output 'BuildLatestModel.cmd [--no-pause]'; exit 0 }
    else { Write-Error ('Unknown option: '+$argument); exit 2 }
}
function Capture-BuildStep([scriptblock]$Action) {
    & $Action 2>&1 | ForEach-Object { [void]$log.AppendLine([string]$_) }
}
try {
    Write-Host 'I94124 雙語語音控制範例：一鍵產生燒錄檔'
    Write-Host '[1/4] 核對 DSMT 與回覆音訊'
    $spec = Read-VoiceIni $root
    Assert-VoiceIniActions $root $spec
    $iniStatus = Get-VoiceIniStatus $spec
    Capture-BuildStep { Write-VoiceOutputs $root (Get-VoiceIniImportOutputs $root $spec $iniStatus) }
    if (-not $iniStatus.Current) {
        Write-Host ('INI 待套用群組：'+($iniStatus.Pending -join '、'))
        Write-Host '匯入檔及對照表：DSMT/Import/README.md；操作：DSMT_OPERATION_GUIDE.md'
        if (-not $pause) { throw '請依 INI 匯入 DSMT、取得發音並儲存工程，再執行 BuildLatestModel.cmd。' }
        [void](Read-Host '請在 DSMT 完成匯入及儲存，完成後回到這裡按 Enter')
        $spec = Read-VoiceIni $root
        Assert-VoiceIniActions $root $spec
        $iniStatus = Get-VoiceIniStatus $spec
        if (-not $iniStatus.Current) { throw ('DSMT 與 INI 尚未一致：'+($iniStatus.Pending -join '、')) }
    }
    Assert-VoiceIniModels $root (Get-VoiceNativeModelState $root $spec)
    Assert-VoiceSavedGroups $root $spec
    $audio = Get-ReplyAudioState $root
    if (-not $audio.Current -and $audio.NeedsSynthesis) {
        if (-not $pause) { throw '回覆文字已變更，請雙擊 BuildLatestModel.cmd，由工具產生 WAV 並引導 Audio Tool 編碼。' }
        Write-Host '正在依回覆文字產生 Windows 國語 WAV。'
        Capture-BuildStep { & (Join-Path $root 'AudioRes/ReplyPhrases/GeneratePhrases.ps1') }
        $audio = Get-ReplyAudioState $root
    }
    if ($audio.TextNeedsWave) { throw '回覆文字已變更，請先替換相應 WAV；錄音格式為 16 kHz、16-bit PCM、單聲道。' }
    if (-not $audio.Current -and -not $audio.EncodedReady) {
        if (-not $pause) { throw '回覆音訊尚未編碼：請在 Audio Tool 開啟 AirconReplies.wba，按 F7，確認 0 errors 後再執行一鍵產生。' }
        Open-ReplyAudioTool $root
        Write-Host '請在 Audio Tool 按 F7，確認 0 errors。'
        [void](Read-Host '完成後回到這裡按 Enter，繼續產生燒錄檔')
        $audio = Get-ReplyAudioState $root
        if (-not $audio.EncodedReady) { throw '尚未找到更新的 Audio Tool 編碼結果；請確認工程路徑及 F7 建置結果。' }
    }
    # Hold the real release lock while the isolated project is built.
    $release = Join-Path $root 'CMDSet/ReleaseV005'
    [void](New-Item -ItemType Directory -Path $release -Force)
    $lockPath = Join-Path $release '.bundle-build.lock'
    Assert-VoicePath $root $lockPath
    $lock = [IO.File]::Open($lockPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    $buildDir = Join-Path $root 'DSMT/Test/Build'
    Assert-VoicePath $root $buildDir
    [void](New-Item -ItemType Directory -Path $buildDir -Force)
    do { $stage = Join-Path $buildDir ('OneClick_' + [guid]::NewGuid().ToString('N').Substring(0,8)) }
    while (Test-Path -LiteralPath $stage)
    [void](New-Item -ItemType Directory -Path $stage)
    [IO.File]::WriteAllText((Join-Path $stage 'OWNED_BY_ONE_CLICK'),'Owned disposable project-build workspace.')
    # Source hashes detect edits made while the staged build is running.
    $snapshots = @{}
    $names = New-Object 'System.Collections.Generic.List[string]'
    foreach ($directory in @('DSMT/Combine/Working','AudioRes/ReplyPhrases','CMDSet/ReleaseV005')) {
        foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $root $directory) -Recurse -File)) {
            if ($file.Name -eq '.bundle-build.lock') { continue }
            if ($directory -eq 'DSMT/Combine/Working' -and $file.Name -notlike 'Group_*' -and $file.Name -ne 'CYBase.mod' -and $file.Extension -ne '.dsmt' -and $file.Name -notlike '*_pack.bin') { continue }
            $names.Add($file.FullName.Substring($root.Length+1))
        }
    }
    foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $root 'DSMT/Input') -Recurse -File)) {
        if ($file.Extension -in @('.json','.txt')) { $names.Add($file.FullName.Substring($root.Length+1)) }
    }
    foreach ($name in @('DSMT/AirconBundleBuilder.exe','DSMT/VoiceMaintenance.ps1','DSMT/VoiceIni.ps1','DSMT/VoiceCommands.ini','DSMT/ReplyAudioBuild.ps1','DSMT/SyncNativeInputs.ps1','DSMT/UpdateVoiceDocs.ps1','DSMT/WriteBuildReport.ps1','DNN_VR/AirconVR.h')) { $names.Add($name) }
    foreach ($name in @('Prj/KEIL/obj/DNN_NuOne_UAC.bin','Prj/KEIL/lst/DNN_NuOne_UAC.map')) {
        if (Test-Path -LiteralPath (Join-Path $root $name)) { $names.Add($name) }
    }
    foreach ($name in ($names | Select-Object -Unique)) {
        $source = Join-Path $root $name; $target = Join-Path $stage $name
        Assert-VoicePath $root $source
        [byte[]]$bytes = Read-VoiceSharedBytes $source
        $snapshots[$name] = Get-VoiceHash $bytes
        [void](New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force)
        [IO.File]::WriteAllBytes($target,$bytes)
        [IO.File]::SetLastWriteTimeUtc($target,(Get-Item -LiteralPath $source).LastWriteTimeUtc)
    }
    Write-Host '[2/4] 同步詞條與核對 Audio Tool 產物'
    Capture-BuildStep {
        $stagedSpec = Read-VoiceIni $stage
        Assert-VoiceIniActions $stage $stagedSpec
        $stagedStatus = Get-VoiceIniStatus $stagedSpec
        if (-not $stagedStatus.Current) { throw '暫存 DSMT 與 INI 不一致，請重試。' }
        $state = Get-VoiceNativeModelState $stage $stagedSpec
        Assert-VoiceIniModels $stage $state
        Assert-VoiceSavedGroups $stage $stagedSpec
        $iniOutputs = [ordered]@{}
        if ($stagedStatus.HasBlank) { $iniOutputs['DSMT/VoiceCommands.ini'] = Get-VoiceIniResolvedBytes $stage $stagedSpec }
        $iniOutputs['DSMT/Input/IniModelState.json'] = Get-VoiceUtf8 (ConvertTo-VoiceJson $state)
        Write-VoiceOutputs $stage $iniOutputs
        $stagedSpec = Read-VoiceIni $stage
        Write-VoiceOutputs $stage (Get-VoiceIniImportOutputs $stage $stagedSpec (Get-VoiceIniStatus $stagedSpec))
    }
    Capture-BuildStep { & (Join-Path $stage 'DSMT/SyncNativeInputs.ps1') }
    Capture-BuildStep { Update-ReplyAudioMetadata $stage }
    Write-Host '[3/4] 產生雙語模型、回覆音訊與測試文件'
    Capture-BuildStep {
        & (Join-Path $stage 'DSMT/AirconBundleBuilder.exe') --no-pause
        if ($LASTEXITCODE -ne 0) {
            $detail = @($log.ToString().Split("`n") | Where-Object { $_ -match '^ERROR:' }) | Select-Object -Last 1
            throw ('請核對 DSMT 儲存或 Audio Tool 編碼結果。'+$detail)
        }
    }
    Capture-BuildStep { & (Join-Path $stage 'DSMT/UpdateVoiceDocs.ps1') -NoPause }
    Capture-BuildStep { & (Join-Path $stage 'DSMT/WriteBuildReport.ps1') }
    # Commit generated files together; Write-VoiceOutputs supplies rollback.
    $outputs = [ordered]@{}
    foreach ($directory in @('DSMT/Input','DSMT/Import','CMDSet/ReleaseV005')) {
        foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $stage $directory) -Recurse -File)) {
            if ($file.Name -eq '.bundle-build.lock') { continue }
            $relative = $file.FullName.Substring($stage.Length+1)
            $outputs[$relative] = [IO.File]::ReadAllBytes($file.FullName)
        }
    }
    $outputs['DSMT/VoiceCommands.ini'] = [IO.File]::ReadAllBytes((Join-Path $stage 'DSMT/VoiceCommands.ini'))
    foreach ($name in @('AudioRes/ReplyPhrases/WavSrc/PhraseSourceManifest.json','AudioRes/ReplyPhrases/AudioBuildManifest.json','DSMT/Test/UserCommandTestGuide.txt','DSMT/Test/CommandMapIDQuickReference.md','DSMT/Test/TaiwanesePronunciation.md')) {
        $outputs[$name] = [IO.File]::ReadAllBytes((Join-Path $stage $name))
    }
    foreach ($name in $snapshots.Keys) {
        if ((Get-VoiceHash (Read-VoiceSharedBytes (Join-Path $root $name))) -ne $snapshots[$name]) { throw ('產生期間來源已變動，請儲存後重試：'+$name) }
    }
    Write-Host '[4/4] 更新發布檔'
    Capture-BuildStep { Write-VoiceOutputs $root $outputs }
    Write-Host 'SUCCESS: 完成 CMDSet/ReleaseV005/AirconDataFlash.bin'
    Write-Host 'APROM：DNN_NuOne_UAC.bin；DataFlash：AirconDataFlash.bin'
    Write-Host '容量與指紋：CMDSet/ReleaseV005/BuildReport.md'
    $exitCode = 0
}
catch {
    [void]$log.AppendLine($_.ToString())
    [void]$log.AppendLine($_.ScriptStackTrace)
    $message = $_.Exception.Message
    if ($stage) { $message = $message.Replace($stage,$root) }
    Write-Host ('未完成：' + $message) -ForegroundColor Red
    Write-Host '發布檔未通過完整產生流程，請依訊息修正後重試。'
}
finally {
    if ($lock) { $lock.Dispose(); Remove-Item -LiteralPath $lockPath -Force }
    if ($stage) {
        try {
            $resolved = [IO.Path]::GetFullPath($stage)
            if ((Split-Path -Parent $resolved) -ne [IO.Path]::GetFullPath((Join-Path $root 'DSMT/Test/Build')) -or -not (Test-Path -LiteralPath (Join-Path $resolved 'OWNED_BY_ONE_CLICK'))) { throw 'Stage ownership mismatch.' }
            $pending = New-Object 'System.Collections.Generic.Stack[string]'; $pending.Push($resolved)
            while ($pending.Count) {
                foreach ($entry in [IO.Directory]::GetFileSystemEntries($pending.Pop())) {
                    $attributes = [IO.File]::GetAttributes($entry)
                    if ($attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Reparse point in disposable stage.' }
                    if ($attributes -band [IO.FileAttributes]::Directory) { $pending.Push($entry) }
                }
            }
            Remove-Item -LiteralPath $resolved -Recurse -Force
        }
        catch { [void]$log.AppendLine('Stage cleanup: '+$_.ToString()); Write-Host ('暫存保留於：'+$stage) }
    }
    try {
        $logDir = Join-Path $root 'DSMT/Test'; [void](New-Item -ItemType Directory -Path $logDir -Force)
        $logPath = Join-Path $logDir ('ProjectBuild_'+(Get-Date -Format 'yyyyMMdd_HHmmss_fff')+'.log')
        [IO.File]::WriteAllText($logPath,$log.ToString(),(New-Object Text.UTF8Encoding -ArgumentList $true))
        Write-Host ('紀錄：'+$logPath)
    }
    catch { Write-Host ('紀錄無法儲存：'+$_.Exception.Message) }
}
if ($pause) { [void](Read-Host '按 Enter 關閉') }
exit $exitCode
