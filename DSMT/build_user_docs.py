"""Generate spoken test steps and a bilingual MapID reference from command records."""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re

from build_command_inputs import DEFAULT_WORKBOOK, read_cells
from build_model_bundle import LANGUAGES, ROOT, load_native
from build_reply_audio import build_audio_image


LABELS = {"Chinese(TWN)": "國語", "Taiwanese": "台語"}
CATEGORY_EN = {"電源": "Power", "模式": "Mode", "溫度": "Temperature", "風量": "Fan",
               "風向": "Swing", "健康": "Health", "音量": "Volume", "定時": "Timer"}
ZONES = {1001: "ROOM", 1002: "LIVING", 1003: "GLOBAL"}


def spoken(record):
    return record.get("spoken_text", record["text"])


def uart(record, zone="ROOM"):
    if record["stage"] == "wake":
        return "[WAKE] language=<實際結果> zone=%s map_id=%d" % (record["value"], record["map_id"])
    text = "[AIRCON] language=<實際結果> zone=%s map_id=%d category=%s command=%s" % (
        zone, record["map_id"], CATEGORY_EN[record["category"]], record["command"])
    if record["unit"]:
        text += " value=%d unit=%s" % (record["value"], record["unit"])
    return text


def command_map_rows(cases):
    rows = {}
    for _, record in cases:
        map_id = record["map_id"]
        if map_id not in rows:
            if record["stage"] == "wake":
                parameter = "zone=" + record["value"]
            elif record["unit"]:
                parameter = "value=%d unit=%s" % (record["value"], record["unit"])
            else:
                parameter = "-"
            rows[map_id] = {
                "map_id": map_id, "category": record["category"],
                "command": record["command"], "parameter": parameter,
                "Chinese(TWN)": [], "Taiwanese": [],
                "reply": "喚醒提示音" if record["stage"] == "wake" else record["expected_reply_text"],
            }
        rows[map_id][record["language"]].append(record["text"])
    return list(rows.values())


def reply_audio_ids():
    build_audio_image(ROOT)
    profile = json.loads((ROOT / "AudioRes/ReplyPhrases/ReplyProfile.json").read_text(encoding="utf-8-sig"))
    return {map_id for phrase in profile["phrases"] for map_id in phrase["map_ids"]}


def render_command_map(cases, seconds):
    lines = ["# 國語／台語命令詞與 MapID 速查表\n\n",
             "同一列的國語及台語說法對應相同 MapID。UART 的 `map_id` 為十進位；`language` 表示接受結果的引擎。\n\n",
             "表中保留模型文字；`nanoe` 在兩語均讀「奈諾伊」。逐句步驟及台語音素見 [UserCommandTestGuide.txt](UserCommandTestGuide.txt)。\n\n",
             "| MapID | 分類 | 國語命令詞 | 台語命令詞 | UART command | 參數／區域 | 國語回覆 |\n",
             "|---|---|---|---|---|---|---|\n"]
    for row in command_map_rows(cases):
        lines.append("| %d | %s | %s | %s | `%s` | %s | %s |\n" % (
            row["map_id"], row["category"], " / ".join(row["Chinese(TWN)"]) or "—",
            " / ".join(row["Taiwanese"]) or "—", row["command"], row["parameter"], row["reply"]))
    lines.append(f"""
## 辨識後的操作

| 結果 | 目前階段 | 下一步 |
|---|---|---|
| 無結果、一般拒絕辨識，尚未逾時 | 控制 | 直接重說命令 |
| 出現 `[AIRCON]`，但功能或數字錯誤 | 控制 | 等提示音播完後重說；該次記為誤辨 |
| `[VR_ERROR] code=-1106` | 等待喚醒 | 重新說喚醒詞，等提示音，再說命令 |
| `[VR_ERROR] code=-1105` | 等待喚醒 | 重新喚醒；若持續發生，檢查音訊處理與遺失 frame |
| `[VR_TIMEOUT]` | 等待喚醒 | 重新說喚醒詞 |
| `[VR_STOPPED]` | 停止辨識 | 保存錯誤紀錄、處理原因後重啟 |

控制等待預設 {seconds} 秒；成功命令及提示音結束後重新計時。`-1106` 表示兩語引擎回傳不同 MapID，該次不輸出控制命令。

## 語音回饋

V005 喚醒播放第 0 筆提示音；控制命令播放「好的，」與功能主句，搭配一次 LED 短閃。兩語命令共用國語回覆。溫度 16～30 度回覆「好的，溫度已設定」，定時 1～12 小時回覆「好的，定時已設定」，實際數值由 UART 核對。

音訊定義見 [ReplyProfile.json](../../AudioRes/ReplyPhrases/ReplyProfile.json)。燒錄使用 V005 的 `DNN_NuOne_UAC.bin` 與 `AirconDataFlash.bin`；流程見 [README](../../README.md)。
""")
    return "".join(lines)


def load_context(release_name):
    manifest = json.loads((ROOT / "DSMT/Input/CommandManifest.json").read_text(encoding="utf-8"))
    records = manifest["records"]
    pronunciations = defaultdict(list)
    summaries = []
    for _, language, folder, filename in LANGUAGES:
        _, summary = load_native(language, folder, filename, records)
        summaries.append(summary)
        project = json.loads((ROOT / summary["project"]).read_text(encoding="utf-8-sig"))
        for group in project["groups"]:
            for words in group.values():
                for word in words:
                    key = (language, word["command"])
                    if word["phoneme"] not in pronunciations[key]:
                        pronunciations[key].append(word["phoneme"])
    release = json.loads((ROOT / "CMDSet" / release_name / "ModelManifest.json").read_text(encoding="utf-8"))
    header = (ROOT / "DNN_VR/AirconVR.h").read_text(encoding="utf-8")
    seconds = int(re.search(r"#define AIRCON_CONTROL_TIMEOUT_SECONDS (\d+)U", header).group(1))
    ordered = sorted(enumerate(records), key=lambda item: (
        item[1]["language"] != "Chinese(TWN)", item[1]["source_row"], item[0]))
    cases = []
    counters = defaultdict(int)
    for _, record in ordered:
        counters[record["language"]] += 1
        prefix = "Z" if record["language"] == "Chinese(TWN)" else "T"
        cases.append((prefix + "%03d" % counters[record["language"]], record))
    return manifest, cases, pronunciations, summaries, release, seconds


def pronunciation_lines(record, pronunciations):
    alternatives = pronunciations[(record["language"], record["text"])]
    if not alternatives:
        return ["  發音狀態：尚無符合指定讀法的已編譯模型；本項 NA。\n"]
    if record["language"] == "Chinese(TWN)" and "nanoe" not in record["text"]:
        return []
    lines = []
    for index, value in enumerate(alternatives, 1):
        syllables = re.sub(r"\d", "", re.sub(r"<[^>]+>", "", value)).replace(".", "-")
        lines.append("  發音候選%d：%s；音節提示：%s\n" % (index, value, syllables))
    if record["text"] == "溫度攑懸":
        lines.append("  『攑懸』本調為 gia̍h-kuân；本專案句中『攑』採台語第三調低降 gia3，完整命令為『溫度攑懸』。\n")
        if "<zh-NAN>un7.too7" in alternatives:
            lines.append("  發音檢查：此候選只有 un7.too7，需在 DSMT 補齊『攑懸』後重產模型。\n")
    if record["text"] == "打開冷氣":
        lines.append("  發音檢查：模型開頭為 thau1；需要調整時，在 DSMT 選取此詞的發音候選並更新。\n")
    return lines


def render_guide(cases, pronunciations, release, seconds):
    audio_ids = reply_audio_ids()
    wakes = [(cid, r) for cid, r in cases if r["stage"] == "wake"]
    commands = [(cid, r) for cid, r in cases if r["stage"] == "control"]
    active_count = sum(bool(pronunciations[(r["language"], r["text"])]) for _, r in cases)
    missing_count = len(cases) - active_count
    active_zhtw = sum(r["language"] == "Chinese(TWN)" and bool(pronunciations[(r["language"], r["text"])]) for _, r in cases)
    active_tw = active_count - active_zhtw
    pair_count = len(wakes) * len(commands)
    active_pairs = len(wakes) * sum(bool(pronunciations[(r["language"], r["text"])]) for _, r in commands)
    missing_pairs = pair_count - active_pairs
    lines = [f"""Panasonic 冷氣國台語完整喚醒與命令測試表
版本 V005　依原 Excel 排序　控制等待 {seconds} 秒

每個案例都明列喚醒詞與命令詞；同功能的別名分開測。全部結果初始為 N 未測。
137 種需求文字＝國語84／台語53；當前已編譯國語{active_zhtw}／台語{active_tw}，其餘{missing_count}項為 NA。
首次使用 V005，更新 APROM 韌體與 AirconDataFlash.bin。已使用相容 V005 韌體時，語音模型或回覆更新可只重燒 AirconDataFlash.bin。
測試者：________ 日期時間：________ 板卡：________ 距離／環境：________
UART log：________　喇叭與外接音訊 Flash：________

一　共同準備與判讀
1. 依 README.md 燒 CMDSet/ReleaseV005/DNN_NuOne_UAC.bin（0x0）與 AirconDataFlash.bin（0x15000），UART0 115200、8N1，從開機存 log。
2. 確認 [APP] release=V005、[AUDIO_READY] clips=23 bps=1.0，以及 [VR_CONFIG] control_timeout_s={seconds} control_timeout_frames={(seconds * 1000 + 29) // 30}。
3. 國語模型 groups=9 controls=8；台語 groups=9 controls=8。
4. 每個獨立案例先按板上 Reset 或重啟，看到 [VR_READY] 後再說話；因此不必每列空等 {seconds} 秒。
5. 喚醒與命令分開說；等喚醒提示音或完整控制回覆播完，再說下一句。
6. 喚醒播放既有提示音；控制命令播放國語完整回覆。每次接受結果有一次 PB0/D4 LED 短閃，控制階段再維持亮燈。
   LED 顏色依板卡。聲光只表示接受某個結果；功能與數字是否正確須核對 UART。
7. 成功後可在 {seconds} 秒內接續命令。一般未辨識成功可直接重說；辨成其他有效命令時，等提示音後重說。
   [VR_TIMEOUT]、-1105 音訊遺失或 -1106 兩語 MapID 衝突會返回等待喚醒，需重新喚醒後再說命令。
   返回等待喚醒時，MCU 與麥克風持續運作。
8. 整句回覆播放時暫停辨識。冷氣電源、定時、音量及靜音仍為 UART 控制請求，應用層另行實作實際動作與狀態。
9. -1104／[VR_STOPPED] 表示停止辨識；保存 [VR_MEMORY] 與錯誤，處理後重啟。-1105／-1106 該次填 E，重新喚醒後續測。
10. language 可能為 ZHTW、TAIWANESE、ZHTW+TAIWANESE；以 MapID、zone、command、value 判讀。

P 正確；M 未辨識；W 辨成別的功能／數值；E 系統錯誤；N 未測；NA 尚無有效模型。
每句測三次，分別記錄原始結果；重試另記一次。
國語回覆定義於 AudioRes/ReplyPhrases/ReplyProfile.json，國台語命令共用；音訊為 16 kHz、NuOne 1.0 bits/sample。
溫度設定回覆「好的，溫度已設定」；定時設定回覆「好的，定時已設定」。UART 仍列出各自的溫度及小時數。

二　台語發音提示使用方式
以下完整保留 DSMT 實際音素與所有候選，另移除數字產生音節順序提示。
音素及尾端數字依 DSMT 台語規則解讀；音節提示供對照整句順序。
按自然口音讀，再對照模型候選與 UART；需要增加讀音時，於 DSMT 更新並儲存，由工具產生有效音素。
多候選 ^1 為索引，口說時讀原命令。nanoe 的產品名讀『奈諾伊』（nano-E）。

三　國語／台語命令詞與 MapID 速查
"""]
    lines.append("MapID\t分類\t國語命令詞\t台語命令詞\tUART command\t參數／區域\t國語回覆\n")
    for row in command_map_rows(cases):
        lines.append("%d\t%s\t%s\t%s\t%s\t%s\t%s\n" % (
            row["map_id"], row["category"], " / ".join(row["Chinese(TWN)"]) or "—",
            " / ".join(row["Taiwanese"]) or "—", row["command"], row["parameter"], row["reply"]))
    lines.append("\n四　137 個逐項完整步驟\n")
    for case_id, record in cases:
        language = record["language"]
        label = LABELS[language]
        wake = next(r for _, r in cases if r["language"] == language and r["map_id"] == 1001)
        command = record
        if record["stage"] == "wake":
            wake = record
            command = next(r for _, r in cases if r["language"] == language and r["map_id"] == 1)
        available = bool(pronunciations[(language, record["text"])])
        lines.append(f"\n[{case_id}] {label}／{record['category']}／Excel {record['source_cell']}／MapID={record['map_id']}\n")
        lines.append(f"  命令詞：{record['text']}；UART command={record['command']}\n")
        lines.append(f"  Excel 原文：{record['source_text']}\n")
        lines.append(f"  1. 按 Reset 或重啟，等 [VR_READY]。\n  2. 用{label}說『{spoken(wake)}』。\n")
        lines.append("  3. 確認 " + uart(wake) + "；等提示音結束及 LED 短閃。\n")
        lines.append(f"  4. 用{label}說『{spoken(command)}』。\n")
        lines.append("  5. 確認 " + uart(command, wake["value"]) + "，並再有一次聲音／LED。\n")
        audio_status = "已編碼於內部音訊" if command["map_id"] in audio_ids else "音訊待製作"
        body = command["map_id"] + 1 if command["map_id"] <= 19 else 21 if command["command"] == "TEMP_SET" else 22
        lines.append(f"  預期國語回覆：{command['expected_reply_text']}（{audio_status}）\n")
        lines.append(f"  回覆紀錄：[AUDIO_REPLY] map_id={command['map_id']} count=2 first=1 last={body}；等整句播完後續說。\n")
        if language == "Taiwanese":
            lines.append("  喚醒發音：" + " ／ ".join(pronunciations[(language, wake["text"])]) + "\n")
        lines.extend(pronunciation_lines(record, pronunciations))
        outcome = "N" if available else "NA"
        lines.append(f"  第1次[{outcome}] 第2次[{outcome}] 第3次[{outcome}]　聲[未測] 燈[未測]\n")
        if not available:
            lines.append("  本列僅保留需求與未來預期值，模型校正前不執行、不計入通過率。\n")
        lines.append("  實際 MapID／參數：________　UART log 行號／備註：________________\n")
    lines.append("\n五　所有喚醒詞與所有命令詞的交叉矩陣\n")
    lines.append(f"以下{pair_count}組涵蓋同語言與跨語言；其中{missing_pairs}組為 NA，其餘{active_pairs}組可測。\n")
    lines.append("每組獨立操作：Reset → 等READY → 依列示語言說喚醒 → 等提示音 → 說命令 → 核對zone與MapID、聲／燈。\n")
    lines.append("發音完整候選依括號案例編號查第三節。先完成主要137項，再按需要完成此完整組合矩陣。\n")
    for index, ((wake_id, wake), (cmd_id, command)) in enumerate(
            ((w, c) for w in wakes for c in commands), 1):
        available = bool(pronunciations[(command["language"], command["text"])])
        outcome = "N" if available else "NA"
        lines.append("[M%03d] %s『%s』(%s) → 等提示 → %s『%s』(%s)；zone=%s MapID=%d；[%s]\n" % (
            index, LABELS[wake["language"]], spoken(wake), wake_id,
            LABELS[command["language"]], spoken(command), cmd_id, wake["value"], command["map_id"], outcome))
    lines.append(f"""
六　等待時間與連續命令
[F01] 國語『房間冷氣』喚醒後安靜等待；約{seconds}秒後才應 [VR_TIMEOUT]，LED熄滅。[N] 秒數____ log____
[F02] 喚醒後約{seconds / 2:g}秒再說國語『開機』，預期仍可成功 map_id=1。[N] log____
[F03] 喚醒後約{max(0.5, seconds - 5):g}秒再說台語『開冷氣』，預期仍可成功 map_id=1。[N] log____
[F04] 命令成功並播完提示後，重新計算一個{seconds}秒控制視窗。[N] log____
[F05] 重啟但不喚醒，只說『設定二十八度』，不應有[AIRCON]或成功提示。[N] log____
[F06] 國語房間冷氣喚醒 → 國語開機 → 台語設定二十八度 → 國語關機；每句等提示結束、間隔不超時。[N] log____
[F07] 只說一次命令，等完整回覆播完後再安靜3秒，不應重複[AIRCON]或因回音觸發命令。[N] log____
[F08] 說溫度16／30及定時1／12小時，核對value邊界；不需等待實際定時小時數。[N] log____
[F09] 分別設定16／26／30度；UART value各自正確，語音均為「好的，溫度已設定」。[N] log____
[F10] 分別設定1／2／12小時；UART value各自正確，語音均為「好的，定時已設定」。[N] log____
[F11] 一次命令只有一筆AIRCON及AUDIO_REPLY、一次LED短閃；「好的，」與主句完整播放，下一句可接續辨識。[N] log____

七　彙整
主要案例已測____/{active_count}（另{missing_count}項NA）；矩陣已測____/{active_pairs}（另{missing_pairs}項NA）；流程____/11。
失敗分類：M____ W____ E____；未解決案例ID與原始log________________________。
冷氣控制介面另行串接；完整回覆音訊依實際控制結果選擇播放內容。
""")
    for name in ("DNN_NuOne_UAC.bin", "AirconDataFlash.bin", "AirconModels.bundle.bin", "AirconReplies.bin"):
        info = release["files"][name]
        lines.append(f"{name}: {info['bytes']} bytes; SHA-256 {info['sha256']}\n")
    return "".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replace-generated", action="store_true")
    args = parser.parse_args()
    context = load_context("ReleaseV005")
    _, cases, pronunciations, summaries, release, seconds = context
    guide = render_guide(cases, pronunciations, release, seconds)
    outputs = {
        ROOT / "DSMT/Test/UserCommandTestGuide.txt": b"\xef\xbb\xbf" + guide.replace("\n", "\r\n").encode("utf-8"),
        ROOT / "DSMT/Test/CommandMapIDQuickReference.md": render_command_map(cases, seconds).encode("utf-8"),
    }
    for path, payload in outputs.items():
        if path.exists() and path.read_bytes() != payload and not args.replace_generated:
            raise FileExistsError("Preserve existing test notes; back up before --replace-generated")
    for path, payload in outputs.items():
        path.write_bytes(payload)
    print("Generated 137 phrase cases, 660 wake/command pairs, 11 workflow cases and 49 MapID rows")


if __name__ == "__main__":
    main()
