$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Read-VoiceJson([string]$Path) {
    Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
}
function Read-VoiceSharedBytes([string]$Path) {
    $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    $memory = New-Object IO.MemoryStream
    try { $stream.CopyTo($memory); return ,$memory.ToArray() }
    finally { $stream.Dispose(); $memory.Dispose() }
}
function Get-VoiceHash([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { [BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function Assert-VoicePath([string]$Root, [string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (-not $full.StartsWith($Root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw ('Path outside project: ' + $full)
    }
    $cursor = $full
    while ($cursor.Length -ge $Root.Length) {
        if (Test-Path -LiteralPath $cursor) {
            if ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw ('Reparse point in output path: ' + $cursor)
            }
        }
        if ($cursor -eq $Root) { break }
        $cursor = Split-Path -Parent $cursor
    }
}
# Write under a lock, verify a backup, and roll back failed replacements.
function Write-VoiceOutputs([string]$Root, [System.Collections.IDictionary]$Outputs) {
    $changed = [ordered]@{}
    foreach ($sourceName in $Outputs.Keys) {
        $name = $sourceName.Replace('\','/')
        $path = Join-Path $Root $name
        Assert-VoicePath $Root $path
        if (-not (Test-Path -LiteralPath $path) -or
            (Get-VoiceHash ([IO.File]::ReadAllBytes($path))) -ne (Get-VoiceHash $Outputs[$sourceName])) {
            $changed[$name] = $Outputs[$sourceName]
        }
    }
    if ($changed.Count -eq 0) { Write-Output 'SUCCESS: generated files are already current.'; return }
    $lockPath = Join-Path $Root '.voice-maintenance.lock'
    Assert-VoicePath $Root $lockPath
    $lock = [IO.File]::Open($lockPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    $before = @{}
    $written = New-Object 'System.Collections.Generic.List[string]'
    try {
        foreach ($name in $changed.Keys) {
            $path = Join-Path $Root $name
            $before[$name] = $null
            if (Test-Path -LiteralPath $path) { $before[$name] = [IO.File]::ReadAllBytes($path) }
        }
        $archiveDir = Join-Path $Root '_Archive'
        Assert-VoicePath $Root $archiveDir
        [void](New-Item -ItemType Directory -Path $archiveDir -Force)
        $backup = Join-Path $archiveDir ('Before_VoiceUpdate_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8) + '.zip')
        $zip = [IO.Compression.ZipFile]::Open($backup, [IO.Compression.ZipArchiveMode]::Create)
        try {
            foreach ($name in $before.Keys) {
                if ($null -eq $before[$name]) { continue }
                $entry = $zip.CreateEntry($name.Replace('\','/'))
                $stream = $entry.Open()
                try { $stream.Write($before[$name], 0, $before[$name].Length) }
                finally { $stream.Dispose() }
            }
        }
        finally { $zip.Dispose() }
        $zip = [IO.Compression.ZipFile]::OpenRead($backup)
        try {
            foreach ($entry in $zip.Entries) {
                $stream = $entry.Open(); $memory = New-Object IO.MemoryStream
                try {
                    $stream.CopyTo($memory)
                    if ((Get-VoiceHash $memory.ToArray()) -ne (Get-VoiceHash $before[$entry.FullName])) {
                        throw 'Generated-file backup verification failed.'
                    }
                }
                finally { $stream.Dispose(); $memory.Dispose() }
            }
        }
        finally { $zip.Dispose() }
        Write-Output ('Verified backup: ' + $backup)
        foreach ($name in $changed.Keys) {
            $path = Join-Path $Root $name
            [void](New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force)
            $written.Add($name)
            [IO.File]::WriteAllBytes($path, $changed[$name])
        }
    }
    catch {
        foreach ($name in $written) {
            $path = Join-Path $Root $name
            if ($null -eq $before[$name]) { Remove-Item -LiteralPath $path -Force }
            else { [IO.File]::WriteAllBytes($path, $before[$name]) }
        }
        throw
    }
    finally { $lock.Dispose(); Remove-Item -LiteralPath $lockPath -Force }
    Write-Output ('SUCCESS: updated ' + $changed.Count + ' generated files.')
}
function Get-VoiceUtf8([string]$Text, [bool]$Bom = $false) {
    $encoding = New-Object Text.UTF8Encoding -ArgumentList $Bom
    return ,([byte[]]($encoding.GetPreamble() + $encoding.GetBytes($Text)))
}
function ConvertTo-VoiceJson($Value) {
    $compact = $Value | ConvertTo-Json -Depth 30 -Compress
    $text = New-Object Text.StringBuilder
    $depth = 0; $quoted = $false; $escaped = $false
    for ($index=0; $index -lt $compact.Length; $index++) {
        $ch = $compact[$index]
        if ($quoted) {
            [void]$text.Append($ch)
            if ($escaped) { $escaped = $false }
            elseif ($ch -eq '\') { $escaped = $true }
            elseif ($ch -eq '"') { $quoted = $false }
            continue
        }
        switch ($ch) {
            '"' { $quoted = $true; [void]$text.Append($ch) }
            { $_ -eq '{' -or $_ -eq '[' } {
                [void]$text.Append($ch); $depth++
                if ($index+1 -lt $compact.Length -and $compact[$index+1] -notin @('}',']')) {
                    [void]$text.Append("`n" + ('  ' * $depth))
                }
            }
            { $_ -eq '}' -or $_ -eq ']' } {
                $depth--
                if ($index -gt 0 -and $compact[$index-1] -notin @('{','[')) {
                    [void]$text.Append("`n" + ('  ' * $depth))
                }
                [void]$text.Append($ch)
            }
            ',' { [void]$text.Append(",`n" + ('  ' * $depth)) }
            ':' { [void]$text.Append(': ') }
            default { if (-not [char]::IsWhiteSpace($ch)) { [void]$text.Append($ch) } }
        }
    }
    $text.ToString() + "`n"
}
function Get-VoiceUtf16([string[]]$Lines) {
    return ,([byte[]]([Text.Encoding]::Unicode.GetPreamble() + [Text.Encoding]::Unicode.GetBytes(($Lines -join "`r`n") + "`r`n")))
}
function Get-VoicePronunciations([string]$Root, $Records) {
    $lookup = @{}
    foreach ($record in $Records) { $lookup[$record.language + '|' + $record.group + '|' + $record.text] = $record }
    $result = @{}
    foreach ($spec in @(@('Chinese(TWN)','ChineseTWN','AirconZHTW.dsmt'), @('Taiwanese','Taiwanese','Taiwanese.dsmt'))) {
        $project = Read-VoiceJson (Join-Path $Root ('DSMT/Combine/Working/' + $spec[1] + '/' + $spec[2]))
        if ($project.language -ne $spec[0]) { throw 'Native project language mismatch.' }
        foreach ($group in $project.groups) {
            foreach ($property in $group.PSObject.Properties) {
                foreach ($word in $property.Value) {
                    $record = $lookup[$spec[0] + '|' + $property.Name + '|' + $word.command]
                    if ($null -eq $record -or $record.map_id -ne $word.CmdMapID -or -not $word.phoneme_id) {
                        throw ('Native command/MapID/pronunciation mismatch: ' + $word.command)
                    }
                    $key = $spec[0] + '|' + $word.command
                    if (-not $result.ContainsKey($key)) { $result[$key] = New-Object 'System.Collections.Generic.List[string]' }
                    if (-not $result[$key].Contains($word.phoneme)) { $result[$key].Add($word.phoneme) }
                }
            }
        }
    }
    foreach ($record in $Records) {
        if (-not $result.ContainsKey($record.language + '|' + $record.text)) {
            throw ('Command has no native pronunciation: ' + $record.text)
        }
    }
    $result
}

function Get-VoiceInputOutputs($Manifest) {
    $records = @($Manifest.records)
    $outputs = [ordered]@{}
    $outputs['DSMT/Input/CommandManifest.json'] = Get-VoiceUtf8 (ConvertTo-VoiceJson $Manifest)
    foreach ($spec in @(@('Chinese(TWN)','ChineseTWN','ZHTW'), @('Taiwanese','Taiwanese','TW'))) {
        $languageRecords = @($records | Where-Object { $_.language -eq $spec[0] })
        $outputs['DSMT/Input/Wake' + $spec[2] + '.txt'] = Get-VoiceUtf16 @($languageRecords | Where-Object stage -eq 'wake' | ForEach-Object text)
        $controls = @($languageRecords | Where-Object stage -eq 'control')
        $outputs['DSMT/Input/Commands' + $spec[2] + '.txt'] = Get-VoiceUtf16 @($controls | ForEach-Object text)
        foreach ($group in ($languageRecords.group | Select-Object -Unique)) {
            $outputs['DSMT/Input/' + $spec[1] + '/' + $group + '.txt'] = Get-VoiceUtf16 @($languageRecords | Where-Object group -eq $group | ForEach-Object text)
        }
    }
    $reference = New-Object 'System.Collections.Generic.List[string]'
    $reference.Add("language`tcategory`tgroup`ttext`tMapID`tcommand`tvalue`tini_section`tini_no")
    foreach ($r in $records) { $reference.Add((@($r.language,$r.category,$r.group,$r.text,$r.map_id,$r.command,$r.value,$r.ini_section,($r.ini_numbers -join ',')) -join "`t")) }
    $outputs['DSMT/Input/MapID_Reference.txt'] = Get-VoiceUtf8 (($reference -join "`r`n") + "`r`n") $true
    $outputs
}
