param([switch]$NoPause)
. (Join-Path $PSScriptRoot 'VoiceMaintenance.ps1')
<#
Generate packaging/user-guide records from saved DSMT words. Existing action
templates supply category/command/value/unit; aliases inherit their MapID action.
#>
. (Join-Path $PSScriptRoot 'VoiceIni.ps1')
$root = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$voiceIni = Read-VoiceIni $root
$manifest = Read-VoiceJson (Join-Path $PSScriptRoot 'Input/CommandManifest.json')
$templates = @($manifest.records)
$profilePath = Join-Path $root 'AudioRes/ReplyPhrases/ReplyProfile.json'
$profile = Read-VoiceJson $profilePath
$previousRelease = $null
$releasePath = Join-Path $root 'CMDSet/ReleaseV005/ModelManifest.json'
if (Test-Path -LiteralPath $releasePath) { $previousRelease = Read-VoiceJson $releasePath }
$replyById = @{}
foreach ($phrase in $profile.phrases) {
    foreach ($map in $phrase.map_ids) {
        if ($phrase.role -ne 'wake') { $replyById[[int]$map] = $profile.prefix + $phrase.text }
    }
}
$records = New-Object 'System.Collections.Generic.List[object]'
$projects = [ordered]@{}
foreach ($spec in @(@('Chinese(TWN)','ChineseTWN','PanasonicAirConZHTW.dsmt'), @('Taiwanese','Taiwanese','Taiwanese.dsmt'))) {
    $relative = 'DSMT/Combine/Working/' + $spec[1] + '/' + $spec[2]
    $projectPath = Join-Path $root $relative
    $project = Read-VoiceJson $projectPath
    if ($project.language -ne $spec[0]) { throw ('DSMT 語言不符：' + $relative) }
    $seen = @{}; $actions = @{}
    foreach ($group in $project.groups) {
        foreach ($property in $group.PSObject.Properties) {
            $previousLanguage = @($previousRelease.languages | Where-Object language -eq $spec[0]) | Select-Object -First 1
            $previousGroup = @($previousLanguage.groups | Where-Object group -eq $property.Name) | Select-Object -First 1
            $hashProperty = $null
            if ($previousLanguage) { $hashProperty = $previousLanguage.model_sha256.PSObject.Properties[$property.Name + '.mod'] }
            if ($previousGroup -and $hashProperty -and $previousGroup.entries -ne @($property.Value).Count) {
                $modelPath = Join-Path (Split-Path -Parent $projectPath) ($property.Name + '.mod')
                if ((Get-VoiceHash ([IO.File]::ReadAllBytes($modelPath))) -eq $hashProperty.Value) {
                    throw ('DSMT 詞條筆數已變更，模型尚未更新；請完整儲存並產生模型：' + $property.Name)
                }
            }
            foreach ($word in $property.Value) {
                $map = [int]$word.CmdMapID
                if ($word.CmdMapID -ne $map) { throw ('MapID 必須是整數：' + $word.command) }
                if ($seen.ContainsKey($word.command)) {
                    if ($seen[$word.command] -ne $map) { throw ('相同詞條對應不同 MapID：' + $word.command) }
                    continue
                }
                $template = @($templates | Where-Object { $_.language -eq $spec[0] -and $_.text -eq $word.command -and $_.map_id -eq $map }) | Select-Object -First 1
                $isNew = $null -eq $template
                if ($isNew) { $template = @($templates | Where-Object { $_.language -eq $spec[0] -and $_.map_id -eq $map }) | Select-Object -First 1 }
                if ($null -eq $template) { $template = @($templates | Where-Object map_id -eq $map) | Select-Object -First 1 }
                if ($null -eq $template) { throw ('新增功能 MapID=' + $map + ' 尚無韌體功能定義；請先完成新功能設定。') }
                if ($property.Name -ne $template.group) { throw ('詞條分類不符：' + $word.command + ' 應放在 ' + $template.group) }
                if (-not $word.phoneme_id -or -not $word.phoneme) { throw ('請在 DSMT 產生有效發音後儲存：' + $word.command) }
                $record = $template | ConvertTo-Json -Depth 20 | ConvertFrom-Json
                $record.language = $spec[0]; $record.text = $word.command
                $iniProject = @($voiceIni.Projects | Where-Object Language -eq $spec[0])[0]
                $iniGroup = @($iniProject.Groups | Where-Object Name -eq $property.Name)[0]
                $iniRows = @($iniGroup.Rows | Where-Object Command -ceq $word.command)
                $record | Add-Member -Force NoteProperty ini_section $iniGroup.Section
                $record | Add-Member -Force NoteProperty ini_numbers @($iniRows.No)
                if ($isNew) {
                    $column = 'B'; if ($spec[0] -eq 'Taiwanese') { $column = 'C' }
                    $record.source_cell = $column + $record.source_row
                    $record.source_text = $word.command
                    $record | Add-Member -Force NoteProperty source_origin 'INI'
                    $record.pronunciation_status = 'native_dsmt'
                }
                if ($record.stage -ne 'wake') {
                    if (-not $replyById.ContainsKey($map)) { throw ('MapID 尚無回覆音訊：' + $map) }
                    $record.expected_reply_text = $replyById[$map]
                    $record | Add-Member -Force NoteProperty reply_override_source 'AudioRes/ReplyPhrases/ReplyProfile.json'
                }
                $record.PSObject.Properties.Remove('spoken_text')
                if ($record.text.Contains('nanoe')) { $record | Add-Member NoteProperty spoken_text ($record.text.Replace('nanoe','奈諾伊').Replace(' ','')) }
                $records.Add($record); $seen[$word.command] = $map; $actions[$map] = $true
            }
        }
    }
    foreach ($map in @($templates | Where-Object language -eq $spec[0] | ForEach-Object map_id | Select-Object -Unique)) {
        if (-not $actions.ContainsKey([int]$map)) { throw ($spec[0] + ' 的 MapID=' + $map + ' 缺少詞條；每個功能至少保留一個說法。') }
    }
    $projects[$spec[0]] = [ordered]@{ path=$relative; sha256=(Get-VoiceHash ([IO.File]::ReadAllBytes($projectPath))) }
}
$manifest.records = $records.ToArray()
$manifest.reply_profile_sha256 = Get-VoiceHash ([IO.File]::ReadAllBytes($profilePath))
$manifest | Add-Member -Force NoteProperty native_projects $projects
$manifest | Add-Member -Force NoteProperty recognition_source 'DSMT/VoiceCommands.ini'
$manifest | Add-Member -Force NoteProperty voice_ini_sha256 $voiceIni.Hash
Write-VoiceOutputs $root (Get-VoiceInputOutputs $manifest)
Write-Output ('Native command texts: ' + $records.Count)
