param([switch]$NoPause)
. (Join-Path $PSScriptRoot 'VoiceMaintenance.ps1')
$root = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$manifest = Read-VoiceJson (Join-Path $PSScriptRoot 'Input/CommandManifest.json')
$records = @($manifest.records)
$pron = Get-VoicePronunciations $root $records
$release = Read-VoiceJson (Join-Path $root 'CMDSet/ReleaseV005/ModelManifest.json')
$header = Get-Content -LiteralPath (Join-Path $root 'DNN_VR/AirconVR.h') -Raw -Encoding UTF8
$seconds = [int]([regex]::Match($header,'#define AIRCON_CONTROL_TIMEOUT_SECONDS (\d+)U').Groups[1].Value)
if ($seconds -le 0) { throw 'Control timeout define not found.' }
$labels = @{ 'Chinese(TWN)'='國語'; Taiwanese='台語' }
$categories = @{ '電源'='Power'; '模式'='Mode'; '溫度'='Temperature'; '風量'='Fan'; '風向'='Swing'; '健康'='Health'; '音量'='Volume'; '定時'='Timer' }
function Spoken($Record) { if ($Record.spoken_text) { $Record.spoken_text } else { $Record.text } }
function Uart($Record, [string]$Zone = 'ROOM') {
    if ($Record.stage -eq 'wake') { return ('[WAKE] language=<實際結果> zone=' + $Record.value + ' map_id=' + $Record.map_id) }
    $line = '[AIRCON] language=<實際結果> zone=' + $Zone + ' map_id=' + $Record.map_id + ' category=' + $categories[$Record.category] + ' command=' + $Record.command
    if ($Record.unit) { $line += ' value=' + $Record.value + ' unit=' + $Record.unit }
    $line
}
function ReplyIndex($Record) {
    if ($Record.stage -eq 'wake') { return '0' }
    $body = 22
    if ($Record.map_id -ge 1 -and $Record.map_id -le 19) { $body = [int]$Record.map_id + 1 }
    elseif ($Record.command -eq 'TEMP_SET') { $body = 21 }
    '1, ' + $body
}
function Phonemes($Record) { @($pron[$Record.language + '|' + $Record.text].ToArray()) }
$cases = New-Object 'System.Collections.Generic.List[object]'
foreach ($language in @('Chinese(TWN)','Taiwanese')) {
    $number = 0
    $selected = @(for ($index=0; $index -lt $records.Count; $index++) {
        if ($records[$index].language -eq $language) {
            [pscustomobject]@{ Row=$records[$index].source_row; Index=$index; Record=$records[$index] }
        }
    })
    $selected = @($selected | Sort-Object Row,Index | ForEach-Object Record)
    foreach ($record in $selected) {
        $number++
        $prefix = 'Z'; if ($language -eq 'Taiwanese') { $prefix = 'T' }
        $cases.Add([pscustomobject]@{ Id=($prefix + $number.ToString('000')); Record=$record })
    }
}
$wakes = @($cases | Where-Object { $_.Record.stage -eq 'wake' })
$commands = @($cases | Where-Object { $_.Record.stage -eq 'control' })
$guide = New-Object 'System.Collections.Generic.List[string]'
$quick = New-Object 'System.Collections.Generic.List[string]'
$roman = New-Object 'System.Collections.Generic.List[string]'
$guide.Add('Panasonic 冷氣國台語逐句測試表 V005')
$guide.Add('控制等待 ' + $seconds + ' 秒；每項測三次。P=正確 M=未辨識 W=誤辨 E=系統錯誤 N=未測。')
$guide.Add('測試者：________ 日期：________ 板卡／距離／環境：________________')
$guide.Add('燒錄 CMDSet/ReleaseV005/DNN_NuOne_UAC.bin 至 0x0，AirconDataFlash.bin 至 0x15000，Verify 後重啟。')
$guide.Add('UART：115200 8N1。開機核對 APP V005、AUDIO_READY、VR_READY languages=2。')
$guide.Add('每項先 Reset，等 VR_READY；說喚醒，等提示音結束，再說命令。接受結果均有 PB0/D4 LED 短閃。')
$guide.Add('完整國語回覆播放時暫停辨識；播完後可在控制視窗內接續命令。一般漏辨可直接重說。')
$guide.Add('VR_TIMEOUT、-1105、-1106 後重新喚醒；VR_STOPPED 先保存錯誤紀錄，再處理原因及重啟。')
$guide.Add('國台語命令共用國語回覆；溫度與定時的數值以 UART 核對。冷氣實際動作由應用層串接。')
$guide.Add('台語逐項保留 DSMT 羅馬拼音與全部候選；音節尾數為 DSMT 聲調標記，口說時不讀數字。')
$guide.Add('nanoe 兩語均讀奈諾伊。溫度攑懸使用 <zh-NAN>un7.too7.gia3.kuan5，其中 gia3 為第三調低降。')
$guide.Add('')
$quick.Add('# 國語／台語命令詞與 MapID 速查表')
$quick.Add('')
$quick.Add('同列別名共用 MapID；nanoe 讀「奈諾伊」。逐句步驟見 [UserCommandTestGuide.txt](UserCommandTestGuide.txt)，完整台語拼音見 [TaiwanesePronunciation.md](TaiwanesePronunciation.md)。')
$quick.Add('')
$quick.Add('| MapID | 分類 | 國語命令詞 | 台語命令詞 | 台語 DSMT 羅馬拼音 | UART command | 參數／區域 | 國語回覆 |')
$quick.Add('|---|---|---|---|---|---|---|---|')
foreach ($map in ($cases.Record.map_id | Select-Object -Unique)) {
    $matching = @($cases.Record | Where-Object map_id -eq $map)
    $first = $matching[0]
    $z = @($matching | Where-Object language -eq 'Chinese(TWN)' | ForEach-Object text) -join ' / '
    $t = @($matching | Where-Object language -eq 'Taiwanese' | ForEach-Object text) -join ' / '
    $p = @($matching | Where-Object language -eq 'Taiwanese' | ForEach-Object {
        $_.text + '：' + ((Phonemes $_ | ForEach-Object { '`' + $_ + '`' }) -join ' ／ ')
    }) -join '<br>'
    $parameter = '—'; $reply = $first.expected_reply_text
    if ($first.stage -eq 'wake') { $parameter = 'zone=' + $first.value; $reply = '喚醒提示音' }
    elseif ($first.unit) { $parameter = 'value=' + $first.value + ' unit=' + $first.unit }
    $quick.Add('| ' + (@($map,$first.category,$z,$t,$p,('`'+$first.command+'`'),$parameter,$reply) -join ' | ') + ' |')
}
$roman.Add('# 台語 DSMT 羅馬拼音與逐句測試索引')
$roman.Add('')
$roman.Add('拼音直接取自 `DSMT/Combine/Working/Taiwanese/Taiwanese.dsmt` 的 `phoneme`，保留全部候選與聲調數字。`<zh-NAN>` 是語言標記，句點分隔音節；候選序號不用念出。')
$roman.Add('')
$roman.Add('先照模型候選發音，再依 UART 的 MapID 判讀結果。不同口音或自然說法可依 INI 新增候選或同 MapID 別名，再由 DSMT 建模；操作見 [DSMT 教學](../../DSMT_OPERATION_GUIDE.md)。nanoe 讀「奈諾伊」，「攑」使用 `gia3`（台語第三調低降）。')
$roman.Add('')
$roman.Add('| 測試編號 | 分類 | 台語詞條 | MapID | DSMT 羅馬拼音（全部候選） |')
$roman.Add('|---|---|---|---|---|')
foreach ($case in $cases) {
    $r = $case.Record
    $label = $labels[$r.language]
    $wake = @($records | Where-Object { $_.language -eq $r.language -and $_.map_id -eq 1001 })[0]
    $command = $r
    if ($r.stage -eq 'wake') { $wake = $r; $command = @($records | Where-Object { $_.language -eq $r.language -and $_.map_id -eq 1 })[0] }
    $sourceRef = 'Excel ' + $r.source_cell
    if ($r.ini_section) { $sourceRef = 'INI [' + $r.ini_section + '] No=' + ($r.ini_numbers -join ',') }
    $guide.Add('[' + $case.Id + '] ' + $label + '／' + $r.category + '／' + $sourceRef + '／MapID=' + $r.map_id)
    $sourceLabel = 'Excel 原文：'
    $displaySource = $r.source_text
    if ($r.source_origin -eq 'DSMT') { $sourceLabel = 'DSMT 詞條：' }
    if ($r.ini_section) { $sourceLabel = '辨識規格：'; $displaySource = $r.text }
    $guide.Add('  命令詞：' + $r.text + '；' + $sourceLabel + $displaySource)
    $guide.Add('  1. 按 Reset，等 [VR_READY]。')
    $guide.Add('  2. 用' + $label + '說「' + (Spoken $wake) + '」。')
    $guide.Add('  3. 核對 ' + (Uart $wake) + '；等提示音結束及 LED 短閃。')
    $guide.Add('  4. 用' + $label + '說「' + (Spoken $command) + '」。')
    $guide.Add('  5. 核對 ' + (Uart $command $wake.value) + '；確認完整回覆及 LED 短閃。')
    $guide.Add('  預期國語回覆：' + $command.expected_reply_text + '；音訊索引：' + (ReplyIndex $command))
    if ($r.language -eq 'Taiwanese') {
        $guide.Add('  喚醒 DSMT 羅馬拼音：' + ((Phonemes $wake) -join ' ／ '))
        $guide.Add('  命令 DSMT 羅馬拼音：' + ((Phonemes $command) -join ' ／ '))
        $roman.Add('| ' + (@($case.Id,$r.category,$r.text,$r.map_id,((Phonemes $r | ForEach-Object { '`'+$_+'`' }) -join '<br>')) -join ' | ') + ' |')
    }
    $guide.Add('  第1次[N] 第2次[N] 第3次[N] 聲[未測] 燈[未測] 實際 MapID／參數：________ UART log：________')
    $guide.Add('')
}
$guide.Add('所有喚醒 × 所有命令交叉測試：每組 Reset → 喚醒 → 等提示 → 命令 → 核對 zone／MapID／回覆／LED。')
$pair = 0
foreach ($wake in $wakes) {
    foreach ($command in $commands) {
        $pair++
        $guide.Add('[M' + $pair.ToString('000') + '] ' + $labels[$wake.Record.language] + '「' + (Spoken $wake.Record) + '」(' + $wake.Id + ') → 等提示 → ' + $labels[$command.Record.language] + '「' + (Spoken $command.Record) + '」(' + $command.Id + ')；zone=' + $wake.Record.value + ' MapID=' + $command.Record.map_id + ' [N]')
    }
}
$flow = @(
    ('喚醒後保持安靜約 ' + $seconds + ' 秒，核對 VR_TIMEOUT 與 LED 熄滅。'),
    ('喚醒後約 ' + [int]($seconds/2) + ' 秒說開機，仍應接受 map_id=1。'),
    ('喚醒後約 ' + ($seconds-5) + ' 秒說台語開冷氣，核對 map_id=1。'),
    ('命令及回覆結束後重新計算 ' + $seconds + ' 秒控制視窗。'),
    '重啟後不喚醒，只說設定二十八度，應無 AIRCON／成功提示。',
    '國語喚醒 → 國語開機 → 台語設定二十八度 → 國語關機，逐次等回覆結束。',
    '一次命令後安靜三秒，應無回音造成的重複 AIRCON。',
    '溫度16／30、定時1／12小時，核對 value 與 unit 邊界。',
    '設定16／26／30度，回覆均為好的，溫度已設定。',
    '設定1／2／12小時，回覆均為好的，定時已設定。',
    '每次僅一筆 AIRCON／AUDIO_REPLY、一次 LED 短閃；好的與主句完整。',
    '說取消定時，核對 map_id=19 command=TIMER_CANCEL、AUDIO_REPLY last=20，回覆好的，定時已取消。',
    '說溫度調低，核對 map_id=8／last=9；回覆中的調讀 tiáo／ㄊㄧㄠˊ，與微調相同。'
)
for ($i=0; $i -lt $flow.Count; $i++) { $guide.Add('[F' + ($i+1).ToString('00') + '] ' + $flow[$i] + ' [N] log：________') }
$guide.Add('彙整：逐句____/' + $cases.Count + '；矩陣____/' + $pair + '；流程____/' + $flow.Count + '。')
foreach ($property in $release.files.PSObject.Properties) { $guide.Add($property.Name + ': ' + $property.Value.bytes + ' bytes; SHA-256 ' + $property.Value.sha256) }
$quick.Add('')
$quick.Add('控制等待預設 ' + $seconds + ' 秒；無結果可重說，VR_TIMEOUT／-1105／-1106 後重新喚醒。辨識成功時核對 UART，再確認國語回覆及 PB0/D4 LED。')
$outputs = [ordered]@{}
$outputs['DSMT/Test/UserCommandTestGuide.txt'] = Get-VoiceUtf8 (($guide -join "`r`n") + "`r`n") $true
$outputs['DSMT/Test/CommandMapIDQuickReference.md'] = Get-VoiceUtf8 (($quick -join "`n") + "`n")
$outputs['DSMT/Test/TaiwanesePronunciation.md'] = Get-VoiceUtf8 (($roman -join "`n") + "`n")
Write-VoiceOutputs $root $outputs
Write-Output ('Cases: ' + $cases.Count + '; wake/command pairs: ' + $pair + '; workflow cases: ' + $flow.Count + '. Python is not required.')
