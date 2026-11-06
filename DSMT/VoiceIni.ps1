$ErrorActionPreference = 'Stop'

<#
INI is the editable recognition specification; DSMT supplies phoneme IDs and
acoustic models. Generate UTF-16LE imports, compare ordered candidates/settings,
and write resolved phonemes back as part of a successful publication.
#>
function Read-VoiceIni([string]$Root) {
    $path = Join-Path $Root 'DSMT/VoiceCommands.ini'
    Assert-VoicePath $Root $path
    $encoding = New-Object Text.UTF8Encoding -ArgumentList $false,$true
    $text = $encoding.GetString((Read-VoiceSharedBytes $path)).TrimStart([char]0xfeff)
    $sections = [ordered]@{}; $section = $null; $lineNo = 0
    foreach ($line in ($text -split '\r\n|\n|\r')) {
        $lineNo++; $value = $line.Trim()
        if (-not $value -or $value.StartsWith(';') -or $value.StartsWith('#')) { continue }
        if ($value -match '^\[([^\[\]]+)\]$') {
            $name = $Matches[1]
            if ($sections.Contains($name)) { throw ('INI 重複區段，第 ' + $lineNo + ' 行：' + $name) }
            $section = [ordered]@{}; $sections[$name] = $section; continue
        }
        $equal = $value.IndexOf('=')
        if ($null -eq $section -or $equal -lt 1) { throw ('INI 格式錯誤，第 ' + $lineNo + ' 行。') }
        $key = $value.Substring(0,$equal).Trim()
        if ($section.Contains($key)) { throw ('INI 重複欄位，第 ' + $lineNo + ' 行：' + $key) }
        $section[$key] = $value.Substring($equal+1).Trim()
    }
    if (-not $sections.Contains('Format') -or $sections['Format'].Count -ne 1 -or $sections['Format']['Version'] -ne '1') { throw 'INI [Format] 必須設定 Version=1。' }
    $projects = New-Object 'System.Collections.Generic.List[object]'
    foreach ($name in $sections.Keys) {
        if ($name -notmatch '^Project\.([A-Za-z][A-Za-z0-9_]*)$') { continue }
        $alias = $Matches[1]; $settings = $sections[$name]
        if ($settings.Count -ne 2 -or -not $settings.Contains('Language') -or -not $settings.Contains('NativeProject')) { throw ('INI 工程欄位應為 Language、NativeProject：' + $name) }
        $native = $settings['NativeProject'].Replace('\','/')
        if ([IO.Path]::IsPathRooted($native) -or $native -notmatch '^DSMT/Combine/Working/.+\.dsmt$') { throw ('INI NativeProject 應為工程內的相對路徑：' + $name) }
        Assert-VoicePath $Root (Join-Path $Root $native)
        Assert-VoicePath (Join-Path $Root 'DSMT/Combine/Working') (Join-Path $Root $native)
        $project = Read-VoiceJson (Join-Path $Root $native)
        if ($project.language -cne $settings['Language']) { throw ('INI 與 DSMT 語系不符：' + $name) }
        $groups = New-Object 'System.Collections.Generic.List[object]'
        foreach ($groupName in $sections.Keys) {
            if (-not $groupName.StartsWith($alias+'.',[StringComparison]::Ordinal)) { continue }
            $group = $groupName.Substring($alias.Length+1)
            if ($group -notmatch '^Group_[1-9][0-9]*_[A-Za-z0-9_]+$') { throw ('INI GROUP 名稱不符：' + $groupName) }
            $rows = New-Object 'System.Collections.Generic.List[object]'; $numbers = @{}; $commands = @{}
            foreach ($key in $sections[$groupName].Keys) {
                $number = 0
                if ($key -notmatch '^(0|[1-9][0-9]*)$' -or -not [int]::TryParse($key,[ref]$number)) { throw ('INI No 必須為 0 起算的整數：' + $groupName + '/' + $key) }
                if ($numbers.ContainsKey($number)) { throw ('INI No 重複：' + $groupName + '/' + $key) }
                $numbers[$number] = $true
                $fields = @($sections[$groupName][$key].Split([char]'|') | ForEach-Object { $_.Trim() })
                if ($fields.Count -ne 7) { throw ('INI 每列須有 7 欄：Command|Reward|SGReward|ExtEnd|MapID|FastSpeech|Phoneme；' + $groupName + '/' + $key) }
                if (-not $fields[0] -or $fields[0] -match '[\x00-\x1f]|\s\^[0-9]+$|\t') { throw ('INI Command 請填原始詞條，不含候選 ^序號：' + $groupName + '/' + $key) }
                $integers = @()
                foreach ($column in @(1,2,4)) {
                    $parsed = 0
                    if ($fields[$column] -notmatch '^-?(0|[1-9][0-9]*)$' -or -not [int]::TryParse($fields[$column],[ref]$parsed)) { throw ('INI Reward、SGReward、MapID 必須為整數：' + $groupName + '/' + $key) }
                    $integers += $parsed
                }
                if ([Math]::Abs([double]$integers[0]) -gt 100 -or [Math]::Abs([double]$integers[1]) -gt 100 -or $integers[2] -lt 0 -or $integers[2] -gt 65535) { throw ('INI Reward/SGReward 範圍 -100～100，MapID 範圍 0～65535：' + $groupName + '/' + $key) }
                $seconds = [decimal]0
                if ($fields[3] -notmatch '^\d+(\.\d{1,2})?$' -or -not [decimal]::TryParse($fields[3],[Globalization.NumberStyles]::AllowDecimalPoint,[Globalization.CultureInfo]::InvariantCulture,[ref]$seconds) -or $seconds -gt [decimal]0.48) { throw ('INI ExtEnd 使用秒，範圍 0～0.48，每格 0.03 秒：' + $groupName + '/' + $key) }
                $frames = $seconds / [decimal]0.03
                if ($frames -ne [Math]::Truncate($frames)) { throw ('INI ExtEnd 必須為 0.03 秒的整數倍：' + $groupName + '/' + $key) }
                if ($fields[5] -cnotin @('Y','N')) { throw ('INI FastSpeech 請填 Y 或 N：' + $groupName + '/' + $key) }
                if ($fields[6] -match '[\x00-\x20]') { throw ('INI Phoneme 不含空白或控制字元：' + $groupName + '/' + $key) }
                if ($fields[6] -and $fields[6] -notmatch '^<[^<>]+>[^<>]+$') { throw ('INI Phoneme 請使用完整 DSMT 音素，包含語言標記：' + $groupName + '/' + $key) }
                if (-not $commands.ContainsKey($fields[0])) { $commands[$fields[0]] = New-Object 'System.Collections.Generic.List[object]' }
                $row = [pscustomobject]@{ No=$number; Command=$fields[0]; Reward=$integers[0]; SGReward=$integers[1]; ExtEnd=$seconds; Frames=[int]$frames; MapID=$integers[2]; FastSpeech=$fields[5]; Phoneme=$fields[6] }
                $commands[$fields[0]].Add($row); $rows.Add($row)
            }
            if (-not $rows.Count) { throw ('INI 群組至少保留一個詞條：' + $groupName) }
            foreach ($command in $commands.Keys) {
                $variants = $commands[$command].ToArray()
                if (@($variants.MapID | Select-Object -Unique).Count -ne 1) { throw ('INI 相同詞條應使用相同 MapID：' + $command) }
                if ($variants.Count -gt 1 -and @($variants | Where-Object { -not $_.Phoneme }).Count) { throw ('INI 發音留白的詞條只需一列：' + $command) }
                if (@($variants.Phoneme | Select-Object -Unique).Count -ne $variants.Count) { throw ('INI 發音候選重複：' + $command) }
            }
            $groups.Add([pscustomobject]@{ Name=$group; Section=$groupName; Rows=@($rows | Sort-Object No) })
        }
        $nativeGroups = @($project.groups | ForEach-Object { $_.PSObject.Properties.Name })
        if (($nativeGroups -join '|') -cne (($groups | ForEach-Object Name) -join '|')) { throw ('INI 群組名稱及排列需與 DSMT 工程一致：' + $alias) }
        $projects.Add([pscustomobject]@{ Alias=$alias; Language=$settings['Language']; Path=$native; Native=$project; Groups=$groups.ToArray() })
    }
    if (-not $projects.Count) { throw 'INI 缺少 [Project.名稱] 設定。' }
    foreach ($name in $sections.Keys) {
        if ($name -eq 'Format' -or @($projects | Where-Object { ('Project.'+$_.Alias) -ceq $name -or @($_.Groups.Section) -ccontains $name }).Count) { continue }
        throw ('INI 未知區段：' + $name)
    }
    [pscustomobject]@{ Projects=$projects.ToArray(); Hash=(Get-VoiceHash (Read-VoiceSharedBytes $path)) }
}

function Test-VoiceIniWord($Row, $Word, [bool]$IncludePhoneme = $true) {
    $fast = 'N'; if ([int]$Word.max_skip_state_num -gt 0) { $fast = 'Y' }
    return ($Row.Command -ceq $Word.command -and $Row.MapID -eq $Word.CmdMapID -and $Row.Reward -eq $Word.reward -and $Row.SGReward -eq $Word.sg_reward -and $Row.Frames -eq $Word.extra_endsil_frame -and $Row.FastSpeech -ceq $fast -and (-not $IncludePhoneme -or $Row.Phoneme -ceq $Word.phoneme) -and $Word.phoneme -and $Word.phoneme_id -and $Word.ipa_encode)
}

# A blank phoneme permits DSMT to expand one phrase into contiguous candidates.
function Get-VoiceIniStatus($Spec) {
    $pending = New-Object 'System.Collections.Generic.List[string]'
    $hasBlank = $false
    foreach ($project in $Spec.Projects) {
        foreach ($group in $project.Groups) {
            $nativeGroup = @($project.Native.groups | Where-Object { $_.PSObject.Properties.Name -ccontains $group.Name })[0]
            $words = @($nativeGroup.($group.Name)); $position = 0; $matches = $true
            foreach ($row in $group.Rows) {
                if ($position -ge $words.Count) { $matches = $false; break }
                if ($row.Phoneme) {
                    if (-not (Test-VoiceIniWord $row $words[$position])) { $matches = $false; break }
                    $position++
                }
                else {
                    $hasBlank = $true; $begin = $position
                    while ($position -lt $words.Count -and (Test-VoiceIniWord $row $words[$position] $false)) { $position++ }
                    if ($position -eq $begin) { $matches = $false; break }
                }
            }
            if (-not $matches -or $position -ne $words.Count) { $pending.Add($project.Alias+'/'+$group.Name) }
        }
    }
    [pscustomobject]@{ Current=($pending.Count -eq 0); Pending=$pending.ToArray(); HasBlank=$hasBlank }
}

function Assert-VoiceIniActions([string]$Root, $Spec) {
    $manifest = Read-VoiceJson (Join-Path $Root 'DSMT/Input/CommandManifest.json')
    $languages = @($Spec.Projects.Language | Sort-Object)
    $expected = @($manifest.records.language | Select-Object -Unique | Sort-Object)
    if (($languages -join '|') -cne ($expected -join '|')) { throw 'INI 語系列表需與韌體的功能定義一致。' }
    foreach ($project in $Spec.Projects) {
        $actions = @{}
        foreach ($group in $project.Groups) {
            foreach ($row in $group.Rows) {
                $template = @($manifest.records | Where-Object { $_.language -ceq $project.Language -and $_.map_id -eq $row.MapID }) | Select-Object -First 1
                if ($null -eq $template) { throw ('INI 新功能 MapID=' + $row.MapID + ' 尚無韌體功能定義。') }
                if ($template.group -cne $group.Name) { throw ('INI MapID=' + $row.MapID + ' 應放在 ' + $template.group) }
                $actions[$row.MapID] = $true
            }
        }
        foreach ($map in @($manifest.records | Where-Object language -eq $project.Language | ForEach-Object map_id | Select-Object -Unique)) {
            if (-not $actions.ContainsKey([int]$map)) { throw ('INI 每個功能至少保留一個詞條：' + $project.Language + '/MapID=' + $map) }
        }
    }
}

function Get-VoiceIniImportOutputs([string]$Root, $Spec, $Status) {
    $outputs = [ordered]@{}
    $instructions = New-Object 'System.Collections.Generic.List[string]'
    $instructions.Add('# INI → DSMT 匯入資料')
    $instructions.Add('')
    $instructions.Add('來源：`DSMT/VoiceCommands.ini`。本目錄由 BuildLatestModel.cmd 更新。')
    $instructions.Add('')
    $instructions.Add('開啟 Working 內的對應原生工程，選擇需要更新的群組，清空該組詞條後，以「批次新增」讀取同名 `.BatchAdd.txt`；完成後儲存工程。群組設定及順序沿用原工程。')
    $instructions.Add('')
    $instructions.Add('`.txt` 含官方群組 XML 設定，供「群組 → 匯入群組」使用；`.BatchAdd.txt` 僅含詞條，供現有群組的「批次新增」使用。兩者均為 UTF-16LE。')
    $instructions.Add('')
    $instructions.Add('詳細操作：[DSMT 操作教學](../../DSMT_OPERATION_GUIDE.md)。')
    $instructions.Add('')
    $instructions.Add('| 語系／群組 | 與已儲存工程比對 | 批次新增檔 |')
    $instructions.Add('|---|---|---|')
    foreach ($project in $Spec.Projects) {
        foreach ($group in $project.Groups) {
            $lines = New-Object 'System.Collections.Generic.List[string]'; $variants = @{}
            foreach ($row in $group.Rows) {
                if (-not $variants.ContainsKey($row.Command)) { $variants[$row.Command] = 0 }
                $label = $row.Command
                if ($variants[$row.Command]) { $label += ' ^' + $variants[$row.Command] }
                $variants[$row.Command]++
                $fields = New-Object 'System.Collections.Generic.List[string]'; $fields.Add($label)
                if ($row.Phoneme) { $fields.Add('-pr'); $fields.Add($row.Phoneme) }
                foreach ($pair in @(@('-reward',$row.Reward),@('-sg',$row.SGReward),@('-extsil',$row.Frames),@('-mapid',$row.MapID),@('-fastspeech',$row.FastSpeech))) {
                    $fields.Add([string]$pair[0]); $fields.Add([string]$pair[1])
                }
                $lines.Add(($fields -join "`t"))
            }
            $nativeText = Join-Path (Split-Path -Parent (Join-Path $Root $project.Path)) ($group.Name+'.txt')
            $header = [Text.Encoding]::Unicode.GetString((Read-VoiceSharedBytes $nativeText)).TrimStart([char]0xfeff).Split(@("`r`n","`n"),[StringSplitOptions]::None)[0]
            $xml = New-Object Xml.XmlDocument; $xml.XmlResolver = $null
            if ($header -match '<!DOCTYPE|<!ENTITY') { throw 'DSMT 群組 XML 不支援 DTD。' }
            $xml.LoadXml($header)
            if ($xml.setting.language -cne $project.Language -or $xml.setting.groupName -cne $group.Name) { throw ('DSMT 群組匯出設定不符：' + $nativeText) }
            $base = 'DSMT/Import/'+$project.Alias+'/'+$group.Name
            $outputs[$base+'.BatchAdd.txt'] = Get-VoiceUtf16 $lines.ToArray()
            $outputs[$base+'.txt'] = Get-VoiceUtf16 (@($header)+$lines.ToArray())
            $state = '一致'; if ($Status.Pending -ccontains ($project.Alias+'/'+$group.Name)) { $state = '**待匯入／儲存**' }
            $instructions.Add('| '+$project.Alias+'/'+$group.Name+' | '+$state+' | ['+$group.Name+']('+$project.Alias+'/'+$group.Name+'.BatchAdd.txt) |')
        }
    }
    $outputs['DSMT/Import/README.md'] = Get-VoiceUtf8 (($instructions -join "`n")+"`n")
    $outputs
}

# Display labels do not alter recognition; phonemes/MapIDs/settings do.
function Get-VoiceNativeModelState([string]$Root, $Spec) {
    $states = [ordered]@{}
    foreach ($project in $Spec.Projects) {
        foreach ($group in $project.Groups) {
            $nativeGroup = @($project.Native.groups | Where-Object { $_.PSObject.Properties.Name -ccontains $group.Name })[0]
            $entries = @($nativeGroup.($group.Name) | ForEach-Object {
                [ordered]@{ phoneme=$_.phoneme; phoneme_id=$_.phoneme_id; map_id=[int]$_.CmdMapID; reward=[int]$_.reward; sg_reward=[int]$_.sg_reward; extra_endsil_frame=[int]$_.extra_endsil_frame; max_skip_state_num=[int]$_.max_skip_state_num }
            })
            $settings = [ordered]@{}
            foreach ($property in ($project.Native.setting.global.($group.Name).PSObject.Properties | Sort-Object Name)) { $settings[$property.Name] = $property.Value }
            $semantic = [ordered]@{ entries=$entries; settings=$settings }
            $model = Join-Path (Split-Path -Parent (Join-Path $Root $project.Path)) ($group.Name+'.mod')
            $states[$project.Alias+'/'+$group.Name] = [ordered]@{ content_sha256=(Get-VoiceHash (Get-VoiceUtf8 (ConvertTo-VoiceJson $semantic))); model_sha256=(Get-VoiceHash (Read-VoiceSharedBytes $model)) }
        }
    }
    [ordered]@{ version=1; groups=$states }
}

function Assert-VoiceSavedGroups([string]$Root, $Spec) {
    foreach ($project in $Spec.Projects) {
        foreach ($group in $project.Groups) {
            $path = Join-Path (Split-Path -Parent (Join-Path $Root $project.Path)) ($group.Name+'.txt')
            $lines = @([Text.Encoding]::Unicode.GetString((Read-VoiceSharedBytes $path)).TrimStart([char]0xfeff) -split '\r\n|\n|\r' | Where-Object { $_.Trim() })
            $nativeGroup = @($project.Native.groups | Where-Object { $_.PSObject.Properties.Name -ccontains $group.Name })[0]
            $words = @($nativeGroup.($group.Name))
            if ($lines.Count -ne $words.Count+1) { throw ('請在 DSMT 完整儲存群組詞條：'+$project.Alias+'/'+$group.Name) }
            for ($index=0; $index -lt $words.Count; $index++) {
                $fields = $lines[$index+1].Split([char]"`t"); $word = $words[$index]; $tokens = @{}
                if ($fields.Count -ne 13 -or $fields[0] -cne $word.show_command) { throw ('DSMT 群組匯出詞條不一致：'+$group.Name) }
                for ($field=1; $field -lt $fields.Count; $field+=2) {
                    if ($tokens.ContainsKey($fields[$field])) { throw ('DSMT 群組匯出欄位重複：'+$group.Name) }
                    $tokens[$fields[$field]]=$fields[$field+1]
                }
                $fast='N'; if ([int]$word.max_skip_state_num -gt 0) { $fast='Y' }
                foreach ($pair in @(@('-pr',$word.phoneme),@('-reward',$word.reward),@('-sg',$word.sg_reward),@('-extsil',$word.extra_endsil_frame),@('-mapid',$word.CmdMapID),@('-fastspeech',$fast))) {
                    if (-not $tokens.ContainsKey($pair[0]) -or $tokens[$pair[0]] -cne [string]$pair[1]) { throw ('DSMT 群組參數尚未完整儲存：'+$project.Alias+'/'+$group.Name+'/'+$word.command) }
                }
            }
        }
    }
}

function Assert-VoiceIniModels([string]$Root, $State) {
    $previous = Read-VoiceJson (Join-Path $Root 'DSMT/Input/IniModelState.json')
    if ($previous.version -ne 1) { throw 'INI 模型狀態版本不符。' }
    foreach ($name in $State.groups.Keys) {
        $old = $previous.groups.PSObject.Properties[$name]
        if ($null -eq $old) { throw ('INI 模型狀態缺少群組：' + $name) }
        $new = $State.groups[$name]
        if ($old.Value.content_sha256 -ne $new.content_sha256 -and $old.Value.model_sha256 -eq $new.model_sha256) { throw ('模型尚未更新，請在 DSMT 儲存並產生模型：' + $name) }
    }
}

function Get-VoiceIniResolvedBytes([string]$Root, $Spec) {
    $path = Join-Path $Root 'DSMT/VoiceCommands.ini'
    $lines = New-Object 'System.Collections.Generic.List[string]'
    $encoding = New-Object Text.UTF8Encoding -ArgumentList $false,$true
    $text = $encoding.GetString((Read-VoiceSharedBytes $path)).TrimStart([char]0xfeff)
    $replacement = @{}
    foreach ($project in $Spec.Projects) {
        foreach ($group in $project.Groups) {
            if (-not @($group.Rows | Where-Object { -not $_.Phoneme }).Count) { continue }
            $nativeGroup = @($project.Native.groups | Where-Object { $_.PSObject.Properties.Name -ccontains $group.Name })[0]
            $rows = New-Object 'System.Collections.Generic.List[string]'; $number = 0
            foreach ($word in @($nativeGroup.($group.Name))) {
                $fast = 'N'; if ([int]$word.max_skip_state_num -gt 0) { $fast = 'Y' }
                $seconds = ([decimal]$word.extra_endsil_frame*[decimal]0.03).ToString('0.##',[Globalization.CultureInfo]::InvariantCulture)
                $rows.Add(($number.ToString()+'='+(@($word.command,$word.reward,$word.sg_reward,$seconds,$word.CmdMapID,$fast,$word.phoneme) -join '|'))); $number++
            }
            $replacement[$group.Section] = $rows.ToArray()
        }
    }
    $active = ''; $inserted = $false
    foreach ($line in ($text.TrimEnd("`r","`n") -split '\r\n|\n|\r')) {
        if ($line.Trim() -match '^\[([^\[\]]+)\]$') { $active = $Matches[1]; $inserted = $false }
        if ($replacement.ContainsKey($active) -and $line -match '^\s*\d+\s*=') {
            if (-not $inserted) { foreach ($row in $replacement[$active]) { $lines.Add($row) }; $inserted = $true }
            continue
        }
        $lines.Add($line)
    }
    return ,(Get-VoiceUtf8 (($lines -join "`r`n")+"`r`n") $true)
}
