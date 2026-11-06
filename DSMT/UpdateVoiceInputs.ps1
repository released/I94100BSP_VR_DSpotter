param([string]$Workbook = '', [switch]$NoPause)
. (Join-Path $PSScriptRoot 'VoiceMaintenance.ps1')
$root = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
if (-not $Workbook) { $Workbook = Join-Path $PSScriptRoot 'Input/Panasonic_AirCon_Full_VoiceCommands.xlsx' }
$Workbook = [IO.Path]::GetFullPath($Workbook)
[byte[]]$bookBytes = Read-VoiceSharedBytes $Workbook
$old = Read-VoiceJson (Join-Path $PSScriptRoot 'Input/CommandManifest.json')
$profilePath = Join-Path $root 'AudioRes/ReplyPhrases/ReplyProfile.json'
$profile = Read-VoiceJson $profilePath
$replyById = @{}
foreach ($phrase in $profile.phrases) {
    if ($phrase.role -eq 'wake') { continue }
    foreach ($map in $phrase.map_ids) { $replyById[[int]$map] = $profile.prefix + $phrase.text }
}
$cells = @{}
$sheetNamespace = 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'
$bookStream = New-Object IO.MemoryStream
$bookStream.Write($bookBytes, 0, $bookBytes.Length)
$bookStream.Position = 0
$zip = New-Object IO.Compression.ZipArchive -ArgumentList @($bookStream, [IO.Compression.ZipArchiveMode]::Read, $false)
function Read-BookXml([string]$Name) {
    $entry = $zip.GetEntry($Name)
    if ($null -eq $entry) { throw ('Workbook entry missing: ' + $Name) }
    $stream = $entry.Open(); $reader = New-Object IO.StreamReader -ArgumentList $stream
    try { [xml]$reader.ReadToEnd() } finally { $reader.Dispose(); $stream.Dispose() }
}
try {
    $strings = @()
    if ($zip.GetEntry('xl/sharedStrings.xml')) {
        $shared = Read-BookXml 'xl/sharedStrings.xml'
        $strings = @($shared.GetElementsByTagName('si', $sheetNamespace) | ForEach-Object { ($_.GetElementsByTagName('t', $sheetNamespace) | ForEach-Object { $_.InnerText }) -join '' })
    }
    $sheet = Read-BookXml 'xl/worksheets/sheet1.xml'
    foreach ($cell in $sheet.GetElementsByTagName('c', $sheetNamespace)) {
        if ($cell.GetElementsByTagName('f', $sheetNamespace).Count) { throw 'Formula cells require a value-only source workbook.' }
        $value = ''
        $values = $cell.GetElementsByTagName('v', $sheetNamespace)
        if ($values.Count) { $value = $values[0].InnerText }
        if ($cell.GetAttribute('t') -eq 's') { $value = $strings[[int]$value] }
        elseif ($cell.GetAttribute('t') -eq 'inlineStr') { $value = ($cell.GetElementsByTagName('t', $sheetNamespace) | ForEach-Object { $_.InnerText }) -join '' }
        $cells[$cell.GetAttribute('r')] = $value
    }
}
finally { $zip.Dispose(); $bookStream.Dispose() }
if ($cells.B1 -ne '國語命令詞' -or $cells.C1 -ne '台語命令詞' -or $cells.D1 -ne '冷氣回覆詞') {
    throw 'Workbook columns do not match the command layout.'
}
$records = New-Object 'System.Collections.Generic.List[object]'
$seenSources = @{}
foreach ($template in $old.records) {
    $source = $template.source_cell
    if ($seenSources.ContainsKey($source)) { continue }
    $seenSources[$source] = $true
    $templates = @($old.records | Where-Object { $_.source_cell -eq $source })
    $row = [int]$source.Substring(1)
    if ($cells['A' + $row] -ne $template.category) { throw ('Excel row/category mismatch: ' + $source) }
    $sourceText = [string]$cells[$source]
    if (-not $sourceText.Trim()) { throw ('Empty command cell: ' + $source) }
    if ($template.command -in @('TEMP_SET','TIMER_OFF')) {
        $items = @($templates | ForEach-Object { @{ Template=$_; Text=$_.text } })
    }
    else {
        $items = @($sourceText.Split('/') | ForEach-Object {
            $text = $_.Trim()
            $selected = $template
            if ($template.command -eq 'WAKE') {
                $selected = @($templates | Where-Object { $_.text -eq $text }) | Select-Object -First 1
                if ($null -eq $selected) { throw ('New wake alias requires an explicit zone/MapID definition: ' + $text) }
            }
            @{ Template=$selected; Text=$text }
        })
    }
    foreach ($item in $items) {
        $record = $item.Template | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $record.text = $item.Text
        $record.PSObject.Properties.Remove('ini_section')
        $record.PSObject.Properties.Remove('ini_numbers')
        if (-not $record.text) { throw ('Empty command alias: ' + $source) }
        $record.source_text = $sourceText
        $record.reply_template = [string]$cells['D' + $row]
        $record.expected_reply_text = $record.reply_template
        $override = $profile.reply_overrides.PSObject.Properties[$record.command]
        if ($null -ne $override) {
            $record.expected_reply_text = $override.Value
            $record | Add-Member -Force NoteProperty reply_override_source 'AudioRes/ReplyPhrases/ReplyProfile.json'
        }
        if ($record.stage -ne 'wake' -and $replyById.ContainsKey([int]$record.map_id)) {
            $record.expected_reply_text = $replyById[[int]$record.map_id]
            $record | Add-Member -Force NoteProperty reply_override_source 'AudioRes/ReplyPhrases/ReplyProfile.json'
        }
        $record.PSObject.Properties.Remove('spoken_text')
        if ($record.text.Contains('nanoe')) {
            $record | Add-Member NoteProperty spoken_text ($record.text.Replace('nanoe','奈諾伊').Replace(' ',''))
        }
        $records.Add($record)
    }
}
$unique = @{}
$meaning = @{}
foreach ($record in $records) {
    $key = $record.language + '|' + $record.text
    if ($unique.ContainsKey($key)) { throw ('Duplicate command: ' + $record.text) }
    $unique[$key] = $true
    $action = $record.command + '|' + $record.value + '|' + $record.unit
    if ($meaning.ContainsKey([int]$record.map_id) -and $meaning[[int]$record.map_id] -ne $action) { throw 'Conflicting MapID actions.' }
    $meaning[[int]$record.map_id] = $action
}
if ($meaning.Count -ne 49) { throw 'Keep the 49 existing actions; new actions require matching firmware/profile changes.' }
$old.records = $records.ToArray()
$old.source_workbook = [IO.Path]::GetFileName($Workbook)
if ($Workbook.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    $old.source_workbook = $Workbook.Substring($root.Length + 1).Replace('\','/')
}
$old.source_sha256 = Get-VoiceHash $bookBytes
$old.reply_profile_sha256 = Get-VoiceHash ([IO.File]::ReadAllBytes($profilePath))
$old | Add-Member -Force NoteProperty recognition_source $old.source_workbook
$old.PSObject.Properties.Remove('voice_ini_sha256')
$outputs = Get-VoiceInputOutputs $old
if ((Get-VoiceHash (Read-VoiceSharedBytes $Workbook)) -ne (Get-VoiceHash $bookBytes)) { throw 'Workbook changed during generation; save and retry.' }
Write-VoiceOutputs $root $outputs
Write-Output ('Commands: ' + $records.Count + '; MapIDs: ' + $meaning.Count + '. Save updated native models in DSMT before packaging.')
