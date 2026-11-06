. (Join-Path $PSScriptRoot 'VoiceMaintenance.ps1')
$root = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$release = Join-Path $root 'CMDSet/ReleaseV005'
$manifest = Read-VoiceJson (Join-Path $release 'ModelManifest.json')
$firmware = $manifest.files.'DNN_NuOne_UAC.bin'
$linker = $null
$mapPath = Join-Path $root 'Prj/KEIL/lst/DNN_NuOne_UAC.map'
if (Test-Path -LiteralPath $mapPath) {
    $map = Get-Content -LiteralPath $mapPath -Raw
    $match = [regex]::Match($map, '(?m)^\s*(\d+)\s+\d+\s+(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+Grand Totals\s*$')
    if ($match.Success -and ([int]$match.Groups[1].Value + [int]$match.Groups[2].Value + [int]$match.Groups[3].Value) -eq $firmware.bytes) {
        $linker = [ordered]@{ Code=[int]$match.Groups[1].Value; RO=[int]$match.Groups[2].Value; RW=[int]$match.Groups[3].Value; ZI=[int]$match.Groups[4].Value }
    }
}
$oldReport = Join-Path $release 'BuildReport.json'
if ($null -eq $linker -and (Test-Path -LiteralPath $oldReport)) {
    $previous = Read-VoiceJson $oldReport
    if ($previous.firmware_sha256 -eq $firmware.sha256) { $linker = $previous.linker }
}
$regions = @(
    [ordered]@{ name='APP'; address='0x00000000'; capacity=86016; used=[int]$firmware.bytes; free=(86016-[int]$firmware.bytes) },
    [ordered]@{ name='國語＋台語模型'; address='0x00015000'; capacity=372736; used=[int]$manifest.files.'AirconModels.bundle.bin'.bytes; free=(372736-[int]$manifest.files.'AirconModels.bundle.bin'.bytes) },
    [ordered]@{ name='回覆音訊'; address='0x00070000'; capacity=65536; used=[int]$manifest.files.'AirconReplies.bin'.bytes; free=(65536-[int]$manifest.files.'AirconReplies.bin'.bytes) }
)
$record = [ordered]@{ schema_version=1; firmware_sha256=$firmware.sha256; regions=$regions; linker=$linker; sram_capacity=196608; files=$manifest.files; languages=$manifest.languages; audio=$manifest.audio }
$lines = New-Object 'System.Collections.Generic.List[string]'
$lines.Add('# 發布檔與記憶體用量')
$lines.Add('')
$lines.Add('此表由一鍵產生工具依本目錄產物更新。1 KiB＝1024 bytes。')
$lines.Add('')
$lines.Add('| 燒錄區域 | 起點 | 檔案 |')
$lines.Add('|---|---|---|')
$lines.Add('| APROM | `0x0` | `DNN_NuOne_UAC.bin` |')
$lines.Add('| DataFlash | `0x15000` | `AirconDataFlash.bin` |')
$lines.Add('')
$lines.Add('| 部位 | 分配 bytes | 使用 bytes | 剩餘 bytes |')
$lines.Add('|---|---:|---:|---:|')
foreach ($region in $regions) { $lines.Add('| ' + (@($region.name,$region.capacity,$region.used,$region.free) -join ' | ') + ' |') }
$lines.Add('')
if ($null -ne $linker) {
    $ram = [int]$linker.RW + [int]$linker.ZI
    $lines.Add('| Keil 項目 | bytes |')
    $lines.Add('|---|---:|')
    foreach ($name in @('Code','RO','RW','ZI')) { $lines.Add('| ' + $name + ' | ' + $linker.$name + ' |') }
    $lines.Add('| RAM 配置 RW＋ZI | ' + $ram + ' |')
    $lines.Add('| RAM 配置後剩餘 | ' + (196608-$ram) + ' |')
    $lines.Add('')
    $lines.Add('RW 在 Flash 與執行時 RAM 各占一份；ZI 只占 RAM。RAM 數值包含啟動檔預留的 stack，實際執行水位及辨識池需求由板上量測。')
}
else { $lines.Add('此 APP 尚無相符的 Keil map 用量資料；Code／RO／RW／ZI 由該版韌體的 Keil Build Output 查閱。') }
$lines.Add('')
$lines.Add('| 產物 | bytes | SHA-256 |')
$lines.Add('|---|---:|---|')
foreach ($property in $manifest.files.PSObject.Properties) { $lines.Add('| `' + $property.Name + '` | ' + $property.Value.bytes + ' | `' + $property.Value.sha256 + '` |') }
$outputs = [ordered]@{}
$outputs['CMDSet/ReleaseV005/BuildReport.json'] = Get-VoiceUtf8 (ConvertTo-VoiceJson $record)
$outputs['CMDSet/ReleaseV005/BuildReport.md'] = Get-VoiceUtf8 (($lines -join "`n") + "`n")
Write-VoiceOutputs $root $outputs
