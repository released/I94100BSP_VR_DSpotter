$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Split-Path -Parent (Split-Path -Parent $PSScriptRoot)))
. (Join-Path $root 'DSMT/VoiceMaintenance.ps1')
. (Join-Path $root 'DSMT/VoiceIni.ps1')
$fixture = Join-Path $PSScriptRoot ('IniFixture_'+[guid]::NewGuid().ToString('N').Substring(0,8))
$cases = New-Object 'System.Collections.Generic.List[object]'
$transcript = New-Object Text.StringBuilder
function Assert-IniTest([bool]$Condition, [string]$Name) {
    if (-not $Condition) { throw ('FAIL '+$Name) }
    $cases.Add([ordered]@{ case=$Name; result='PASS' })
    [void]$transcript.AppendLine('PASS '+$Name); Write-Output ('PASS '+$Name)
}
function Set-TestIni([string]$Text) { [IO.File]::WriteAllBytes((Join-Path $fixture 'DSMT/VoiceCommands.ini'),(Get-VoiceUtf8 $Text $true)) }
function Test-BadIni([string]$Text,[string]$Pattern,[string]$Name,[bool]$Actions=$false) {
    Set-TestIni $Text; $caught = $false
    try { $value=Read-VoiceIni $fixture; if ($Actions) { Assert-VoiceIniActions $fixture $value } }
    catch { if ($_.Exception.Message -notmatch $Pattern) { throw }; $caught=$true }
    Assert-IniTest $caught $Name
}
try {
    Assert-VoicePath $root $fixture
    [void](New-Item -ItemType Directory -Path $fixture)
    [IO.File]::WriteAllText((Join-Path $fixture 'OWNED_BY_INI_TEST'),'Disposable INI test fixture.')
    foreach ($name in @('DSMT/VoiceCommands.ini','DSMT/Input/CommandManifest.json','DSMT/Input/IniModelState.json')) {
        $target=Join-Path $fixture $name
        [void](New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force)
        [IO.File]::WriteAllBytes($target,(Read-VoiceSharedBytes (Join-Path $root $name)))
    }
    foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $root 'DSMT/Combine/Working') -Recurse -File)) {
        if ($file.Extension -ne '.dsmt' -and $file.Name -notlike 'Group_*.txt' -and $file.Name -notlike 'Group_*.mod') { continue }
        $name=$file.FullName.Substring($root.Length+1); $target=Join-Path $fixture $name
        [void](New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force)
        [IO.File]::WriteAllBytes($target,(Read-VoiceSharedBytes $file.FullName))
    }
    $original=[IO.File]::ReadAllText((Join-Path $fixture 'DSMT/VoiceCommands.ini'),[Text.Encoding]::UTF8)
    $spec=Read-VoiceIni $fixture; $status=Get-VoiceIniStatus $spec
    Assert-VoiceIniActions $fixture $spec
    $count=0; foreach ($project in $spec.Projects) { foreach ($group in $project.Groups) { $count += $group.Rows.Count } }
    Assert-IniTest ($spec.Projects.Count -eq 2 -and $count -eq 299 -and $status.Current) 'Both languages and all 299 native pronunciation rows match the INI'
    $outputs=Get-VoiceIniImportOutputs $fixture $spec $status; $same=0
    foreach ($project in $spec.Projects) {
        foreach ($group in $project.Groups) {
            $actual=$outputs['DSMT/Import/'+$project.Alias+'/'+$group.Name+'.txt']
            $native=Read-VoiceSharedBytes (Join-Path (Split-Path -Parent (Join-Path $fixture $project.Path)) ($group.Name+'.txt'))
            if ((Get-VoiceHash $actual) -eq (Get-VoiceHash $native)) { $same++ }
        }
    }
    Assert-IniTest ($outputs.Count -eq 37 -and $same -eq 18) 'All 18 generated UTF-16LE group files are byte-identical to DSMT exports'
    Assert-VoiceIniModels $fixture (Get-VoiceNativeModelState $fixture $spec)
    Assert-VoiceSavedGroups $fixture $spec
    Assert-IniTest $true 'Initial semantic fingerprints match the saved model baseline'
    Test-BadIni ($original.Replace('0=開機|','0=開機|' + "`r`n" + '0=開機|')) '重複欄位' 'Duplicate No is rejected'
    Test-BadIni ($original.Replace('開機|0|0|0|1|N|','開機|101|0|0|1|N|')) '範圍' 'Out-of-range Reward is rejected'
    Test-BadIni ($original.Replace('開機|0|0|0|1|N|','開機|0|0|0|1.5|N|')) '整數' 'Fractional MapID is rejected'
    Test-BadIni ($original.Replace('開機|0|0|0|1|N|','開機|0|0|0|1|X|')) 'FastSpeech' 'Unknown FastSpeech flag is rejected'
    Test-BadIni ($original.Replace('開機|0|0|0|1|N|','開機|0|0|0.005|1|N|')) 'ExtEnd' 'Sub-frame Extra Ending is rejected'
    Test-BadIni ($original.Replace('開機|0|0|0|1|N|','開機|0|0|0|9999|N|')) '韌體功能' 'An undefined firmware MapID is rejected' $true
    Test-BadIni ($original.Replace('開機|0|0|0|1|N|','開機|0|0|0|3|N|')) '應放在' 'A valid MapID in the wrong category is rejected' $true
    Test-BadIni ($original.Replace('[ChineseTWN.Group_2_Power]','[ChineseTWN.Group_2_Powe]')) '名稱及排列' 'Unknown or missing group names are rejected'
    Test-BadIni ($original.Replace('Language=Taiwanese','Language=Chinese(TWN)')) '語系不符' 'A language mismatch is rejected'
    Test-BadIni ($original.Replace('NativeProject=DSMT/Combine/Working/ChineseTWN/AirconZHTW.dsmt','NativeProject=../foreign.dsmt')) '相對路徑' 'Native project paths outside the workspace are rejected'
    Set-TestIni ($original.Replace('開機|0|0|0|1|N|','開機|-2|3|0.12|1|Y|'))
    $changed=Read-VoiceIni $fixture; $row=$changed.Projects[0].Groups[1].Rows[0]
    $imports=Get-VoiceIniImportOutputs $fixture $changed (Get-VoiceIniStatus $changed)
    $batch=[Text.Encoding]::Unicode.GetString($imports['DSMT/Import/ChineseTWN/Group_2_Power.BatchAdd.txt'])
    Assert-IniTest ($row.Frames -eq 4 -and $batch.Contains("-reward`t-2`t-sg`t3`t-extsil`t4`t-mapid`t1`t-fastspeech`tY")) 'Reward, SGReward, seconds-to-30ms-frames and FastSpeech serialize into the DSMT token format'
    $added=$original.Replace('[ChineseTWN.Group_3_Mode]',"6=開啟冷氣|0|0|0|1|N|`r`n`r`n[ChineseTWN.Group_3_Mode]")
    Set-TestIni $added; $changed=Read-VoiceIni $fixture; $state=Get-VoiceIniStatus $changed
    $imports=Get-VoiceIniImportOutputs $fixture $changed $state
    $batch=[Text.Encoding]::Unicode.GetString($imports['DSMT/Import/ChineseTWN/Group_2_Power.BatchAdd.txt'])
    Assert-IniTest (-not $state.Current -and $state.Pending.Count -eq 1 -and $state.Pending[0] -eq 'ChineseTWN/Group_2_Power' -and $batch.Contains("開啟冷氣`t-reward") -and -not $batch.Contains("開啟冷氣`t-pr")) 'A new blank-pronunciation alias requests DSMT pronunciation in exactly its own group'
    $blank=[regex]::Replace($original,'(?m)^0=開冷氣\|0\|0\|0\|1\|N\|<zh-NAN>[^\r\n]+','0=開冷氣|0|0|0|1|N|')
    $blank=[regex]::Replace($blank,'(?m)^1=開冷氣\|0\|0\|0\|1\|N\|<zh-NAN>[^\r\n]+\r?\n','')
    Set-TestIni $blank; $changed=Read-VoiceIni $fixture; $state=Get-VoiceIniStatus $changed
    Assert-IniTest ($state.Current -and $state.HasBlank) 'One blank pronunciation can match all valid contiguous DSMT-generated candidates'
    [IO.File]::WriteAllBytes((Join-Path $fixture 'DSMT/VoiceCommands.ini'),(Get-VoiceIniResolvedBytes $fixture $changed))
    $resolved=Read-VoiceIni $fixture; $state=Get-VoiceIniStatus $resolved
    Assert-IniTest ($state.Current -and -not $state.HasBlank -and $resolved.Projects[1].Groups[1].Rows.Count -eq 7) 'Resolved candidates and actual phonemes are written back into the canonical INI'
    Set-TestIni $original
    $nativePath=Join-Path $fixture 'DSMT/Combine/Working/ChineseTWN/AirconZHTW.dsmt'
    $native=Read-VoiceJson $nativePath
    $native.groups[1].Group_2_Power[0].reward=1
    [IO.File]::WriteAllBytes($nativePath,(Get-VoiceUtf8 (ConvertTo-VoiceJson $native)))
    Set-TestIni ($original.Replace('開機|0|0|0|1|N|','開機|1|0|0|1|N|'))
    $changed=Read-VoiceIni $fixture; $state=Get-VoiceIniStatus $changed; $caught=$false
    try { Assert-VoiceIniModels $fixture (Get-VoiceNativeModelState $fixture $changed) } catch { if ($_.Exception.Message -notmatch '模型尚未更新') { throw }; $caught=$true }
    Assert-IniTest ($state.Current -and $caught) 'A parameter-only edit with the old model is rejected even with matching INI/native words'
    $caught=$false
    try { Assert-VoiceSavedGroups $fixture $changed } catch { if ($_.Exception.Message -notmatch '參數尚未完整儲存') { throw }; $caught=$true }
    Assert-IniTest $caught 'A saved text export with stale Reward values is rejected'
    [IO.File]::WriteAllBytes((Join-Path $fixture 'DSMT/VoiceCommands.ini'),[byte[]]@(255,254,255))
    $caught=$false; try { Read-VoiceIni $fixture | Out-Null } catch { $caught=$true }
    Assert-IniTest $caught 'Invalid UTF-8 is rejected without replacing source files'
    [IO.File]::WriteAllBytes((Join-Path $PSScriptRoot 'VoiceIniTests.json'),(Get-VoiceUtf8 (ConvertTo-VoiceJson ([ordered]@{ result='PASS'; checks=$cases.Count; cases=$cases.ToArray(); limits=@('DSMT UI import and acoustic recognition require a separate interactive/hardware check.') }))))
}
finally {
    [IO.File]::WriteAllText((Join-Path $PSScriptRoot 'VoiceIniTests.log'),$transcript.ToString(),(New-Object Text.UTF8Encoding -ArgumentList $true))
    if (Test-Path -LiteralPath $fixture) {
        $resolved=[IO.Path]::GetFullPath($fixture)
        if ((Split-Path -Parent $resolved) -ne $PSScriptRoot -or -not (Test-Path -LiteralPath (Join-Path $resolved 'OWNED_BY_INI_TEST'))) { throw 'Unexpected INI fixture ownership.' }
        $pending=New-Object 'System.Collections.Generic.Stack[string]'; $pending.Push($resolved)
        while ($pending.Count) {
            foreach ($entry in [IO.Directory]::GetFileSystemEntries($pending.Pop())) {
                $attributes=[IO.File]::GetAttributes($entry)
                if ($attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Reparse point in INI fixture.' }
                if ($attributes -band [IO.FileAttributes]::Directory) { $pending.Push($entry) }
            }
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
Write-Output 'PASS owned INI fixture removed'
