param(
    [string]$VoiceName = '',
    [ValidateRange(-10, 10)][int]$Rate = 0,
    [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$profilePath = Join-Path $PSScriptRoot 'ReplyProfile.json'
$profile = Get-Content -LiteralPath $profilePath -Raw -Encoding UTF8 | ConvertFrom-Json
if (-not $VoiceName) { $VoiceName = $profile.voice }
if (-not $PSBoundParameters.ContainsKey('Rate')) { $Rate = $profile.rate }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot 'WavSrc' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$existing = Test-Path -LiteralPath $OutputDirectory
Add-Type -AssemblyName System.Speech
Add-Type -TypeDefinition @'
using System;
using System.IO;
public static class ReplyPcm {
    public static byte[] Trim(byte[] pcm, double db, int paddingMs) {
        int count = pcm.Length/2, first = -1, end = 0;
        double threshold = 32768.0*Math.Pow(10,db/20.0);
        for (int start=0;start<count;start+=160) {
            int stop=Math.Min(start+160,count);
            long sum=0;
            for(int i=start;i<stop;++i) {
                short value=(short)(pcm[2*i] | (pcm[2*i+1]<<8));
                sum+=(long)value*value;
            }
            if(Math.Sqrt((double)sum/(stop-start))>=threshold) {
                if(first<0) first=start;
                end=stop;
            }
        }
        if(first<0) throw new InvalidDataException("No active speech");
        first=Math.Max(0,first-paddingMs*16);
        end=Math.Min(count,end+paddingMs*16);
        byte[] trimmed=new byte[(end-first)*2];
        Buffer.BlockCopy(pcm,first*2,trimmed,0,trimmed.Length);
        return trimmed;
    }
    public static void WriteWave(string path,byte[] pcm) {
        using(BinaryWriter writer=new BinaryWriter(File.Create(path))) {
            writer.Write(System.Text.Encoding.ASCII.GetBytes("RIFF"));
            writer.Write(36+pcm.Length);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt "));
            writer.Write(16);writer.Write((short)1);writer.Write((short)1);
            writer.Write(16000);writer.Write(32000);writer.Write((short)2);writer.Write((short)16);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("data"));
            writer.Write(pcm.Length);writer.Write(pcm);
        }
    }
}
'@
$records = (Get-Content -LiteralPath (Join-Path $root 'DSMT\Input\CommandManifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json).records
$expected = @($records.map_id | Sort-Object -Unique)
$actual = @($profile.phrases | ForEach-Object { $_.map_ids } | Sort-Object -Unique)
if (($expected -join ',') -ne ($actual -join ',')) { throw 'Reply profile does not cover the current MapIDs.' }
$format = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo -ArgumentList @(
    16000,[System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,[System.Speech.AudioFormat.AudioChannel]::Mono)
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$entries = New-Object 'System.Collections.Generic.List[object]'
try {
    $synth.SelectVoice($VoiceName)
    $synth.Rate = $Rate
    if ($existing) {
        $oldManifest = Join-Path $OutputDirectory 'PhraseSourceManifest.json'
        if (-not (Test-Path -LiteralPath $oldManifest)) { throw 'Preserve existing audio; select a new -OutputDirectory.' }
        $old = Get-Content -LiteralPath $oldManifest -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($entry in $old.entries) {
            if ([IO.Path]::GetFileName($entry.file) -ne $entry.file) { throw 'Invalid existing source filename.' }
            $path = Join-Path $OutputDirectory $entry.file
            if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.sha256) {
                throw ('Preserve changed recording: '+$entry.file+'; select a new -OutputDirectory.')
            }
        }
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $archiveDir = Join-Path $root '_Archive'
        [void](New-Item -ItemType Directory -Path $archiveDir -Force)
        $backup = Join-Path $archiveDir ('Before_ReplyWav_'+(Get-Date -Format 'yyyyMMdd_HHmmss')+'_'+[guid]::NewGuid().ToString('N').Substring(0,8)+'.zip')
        [IO.Compression.ZipFile]::CreateFromDirectory($OutputDirectory,$backup)
        $zip = [IO.Compression.ZipFile]::OpenRead($backup)
        try {
            foreach ($entry in $zip.Entries) {
                if (-not $entry.Name) { continue }
                $stream = $entry.Open()
                $sha = [Security.Cryptography.SHA256]::Create()
                try { $hash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','') }
                finally { $stream.Dispose(); $sha.Dispose() }
                $path = Join-Path $OutputDirectory $entry.FullName
                if ($hash -ne (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash) { throw 'Audio source backup verification failed.' }
            }
        }
        finally { $zip.Dispose() }
        Write-Output ('Verified backup: '+$backup)
    }
    else { [void](New-Item -ItemType Directory -Path $OutputDirectory) }
    foreach ($phrase in ($profile.phrases | Sort-Object index)) {
        if ([IO.Path]::GetFileName($phrase.file) -ne $phrase.file) { throw 'Invalid phrase filename.' }
        $path = Join-Path $OutputDirectory $phrase.file
        if ($phrase.copy_source) {
            Copy-Item -LiteralPath (Join-Path $root $phrase.copy_source) -Destination $path
        }
        else {
            $stream = New-Object IO.MemoryStream
            try {
                $synth.SetOutputToAudioStream($stream,$format)
                $synth.Speak($phrase.text)
                $pcm = $stream.ToArray()
                $synth.SetOutputToNull()
            }
            finally { $stream.Dispose() }
            $pcm = [ReplyPcm]::Trim($pcm,$profile.trim_threshold_dbfs,$profile.edge_padding_ms)
            [ReplyPcm]::WriteWave($path,$pcm)
        }
        $entries.Add([pscustomobject][ordered]@{
            index=$phrase.index;file=$phrase.file;role=$phrase.role;command=$phrase.command
            map_ids=@($phrase.map_ids);spoken_text=$phrase.text
            bytes=(Get-Item -LiteralPath $path).Length
            sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        })
        Write-Output ('Generated '+$phrase.file)
    }
    $manifest = [ordered]@{
        schema_version=1;release='V005';sample_rate=16000;bits_per_sample=16;channels=1
        voice=$VoiceName;rate=$Rate;nuone_bits_per_sample=1.0;prefix_gap_ms=$profile.prefix_gap_ms
        source_profile='AudioRes/ReplyPhrases/ReplyProfile.json'
        source_profile_sha256=(Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash.ToLowerInvariant()
        entries=$entries.ToArray()
    }
    [IO.File]::WriteAllText((Join-Path $OutputDirectory 'PhraseSourceManifest.json'),($manifest|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding -ArgumentList $true))
    $newline = [Environment]::NewLine
    $uid = '[Audio]' + $newline
    foreach($phrase in ($profile.phrases | Sort-Object index)) {
        $uid += ('defaud '+[IO.Path]::GetFileNameWithoutExtension($phrase.file)+' <ext=0, format=NUO, bitrate=1.0>'+$newline)
    }
    [IO.File]::WriteAllText((Join-Path $OutputDirectory 'AirconReplies.uid'),$uid,[Text.Encoding]::ASCII)
    Write-Output ('SUCCESS: '+$entries.Count+' phrase WAV files and AirconReplies.uid')
}
finally { $synth.Dispose() }
