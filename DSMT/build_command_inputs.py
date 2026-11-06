"""Build DSMT text inputs and the expected MapID manifest from the source workbook."""

import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_WORKBOOK = ROOT / "DSMT/Input/Panasonic_AirCon_Full_VoiceCommands.xlsx"
NS = {"s": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
REPLY_PROFILE = Path(__file__).resolve().parent.parent / "AudioRes/ReplyPhrases/ReplyProfile.json"
FIXED_ROWS = (
    (3, 1, "POWER_ON"), (4, 2, "POWER_OFF"),
    (5, 3, "MODE_COOL"), (6, 4, "MODE_HEAT"),
    (7, 5, "MODE_DRY"), (8, 6, "MODE_AUTO"),
    (9, 7, "TEMP_UP"), (10, 8, "TEMP_DOWN"),
    (12, 9, "FAN_UP"), (13, 10, "FAN_DOWN"),
    (14, 11, "FAN_AUTO"), (15, 12, "SWING_AUTO"),
    (16, 13, "SWING_HOLD"), (17, 14, "NANOE_ON"),
    (18, 15, "NANOE_OFF"), (19, 16, "VOICE_VOL_UP"),
    (20, 17, "VOICE_VOL_DOWN"), (21, 18, "VOICE_MUTE"),
    (23, 19, "TIMER_CANCEL"),
)
WAKE_IDS = {"房間冷氣": (1001, "ROOM"), "客廳冷氣": (1002, "LIVING"),
            "國際冷氣": (1003, "GLOBAL")}
NUMBER_WORDS = {1: "一", 2: "兩", 3: "三", 4: "四", 5: "五", 6: "六",
                7: "七", 8: "八", 9: "九", 10: "十", 11: "十一", 12: "十二"}
GROUPS = (
    ("Group_1_Wake", "喚醒", ("WAKE",)),
    ("Group_2_Power", "電源", ("POWER_ON", "POWER_OFF")),
    ("Group_3_Mode", "模式", ("MODE_COOL", "MODE_HEAT", "MODE_DRY", "MODE_AUTO")),
    ("Group_4_Temperature", "溫度", ("TEMP_UP", "TEMP_DOWN", "TEMP_SET")),
    ("Group_5_Fan", "風量", ("FAN_UP", "FAN_DOWN", "FAN_AUTO")),
    ("Group_6_Swing", "風向", ("SWING_AUTO", "SWING_HOLD")),
    ("Group_7_Health", "健康", ("NANOE_ON", "NANOE_OFF")),
    ("Group_8_Volume", "音量", ("VOICE_VOL_UP", "VOICE_VOL_DOWN", "VOICE_MUTE")),
    ("Group_9_Timer", "定時", ("TIMER_OFF", "TIMER_CANCEL")),
)


def read_cells(path):
    with zipfile.ZipFile(path) as book:
        workbook = ET.fromstring(book.read("xl/workbook.xml"))
        sheets = workbook.findall("s:sheets/s:sheet", NS)
        if len(sheets) != 1 or sheets[0].get("name") != "完整語音指令與回覆":
            raise ValueError("Unexpected workbook sheets; review source layout first")
        strings = []
        if "xl/sharedStrings.xml" in book.namelist():
            strings = ["".join(node.itertext()) for node in
                       ET.fromstring(book.read("xl/sharedStrings.xml")).findall("s:si", NS)]
        sheet = ET.fromstring(book.read("xl/worksheets/sheet1.xml"))
        cells = {}
        for cell in sheet.findall("s:sheetData/s:row/s:c", NS):
            if cell.find("s:f", NS) is not None:
                raise ValueError("Formula cells require source review")
            inline = cell.find("s:is", NS)
            value = ("".join(inline.itertext()) if inline is not None else
                     cell.findtext("s:v", default="", namespaces=NS))
            if cell.get("t") == "s":
                value = strings[int(value)]
            cells[cell.get("r")] = value
    expected = {"B1": "國語命令詞", "C1": "台語命令詞", "D1": "冷氣回覆詞"}
    if any(cells.get(key) != value for key, value in expected.items()):
        raise ValueError("Unexpected workbook column labels")
    return cells


def temperature_word(value):
    digits = "零一二三四五六七八九"
    if value < 20:
        return "十" + digits[value - 10]
    tens, ones = divmod(value, 10)
    return digits[tens] + "十" + (digits[ones] if ones else "")


def make_record(language, group, text, map_id, command, value, source_cell, unit=None):
    matches = [(name, label) for name, label, commands in GROUPS if command in commands]
    if len(matches) != 1:
        raise ValueError("Unclassified command: " + command)
    category_group, category = matches[0]
    return {"language": language, "group": category_group, "category": category,
            "stage": "wake" if command == "WAKE" else "control", "text": text,
            "map_id": map_id, "command": command, "value": value, "unit": unit,
            "source_cell": source_cell,
            "pronunciation_status": ("nanoe_reading_confirmed_requires_dsmt" if "nanoe" in text
                                     else "not_yet_verified_in_dsmt")}


def build_records(cells):
    records = []
    for language, column in (("Chinese(TWN)", "B"), ("Taiwanese", "C")):
        for text in cells[column + "2"].split("/"):
            text = text.strip()
            map_id, zone = WAKE_IDS[text]
            records.append(make_record(language, "Group_1", text, map_id, "WAKE",
                                       zone, column + "2"))
        for row, map_id, command in FIXED_ROWS:
            for text in cells[column + str(row)].split("/"):
                records.append(make_record(language, "Group_2", text.strip(), map_id,
                                           command, None, column + str(row)))
        for value in range(16, 31):
            records.append(make_record(language, "Group_2",
                                       "設定" + temperature_word(value) + "度",
                                       100 + value, "TEMP_SET", value, column + "11", "C"))
        for hours in range(1, 13):
            phrases = [NUMBER_WORDS[hours] + "小時後關機"]
            if language == "Chinese(TWN)":
                phrases.insert(0, "定時" + NUMBER_WORDS[hours] + "小時關機")
            for text in phrases:
                records.append(make_record(language, "Group_2", text, 200 + hours,
                                           "TIMER_OFF", hours, column + "22", "h"))
    for record in records:
        row = int(record["source_cell"][1:])
        if cells.get("A" + str(row)) != record["category"]:
            raise ValueError("Excel category mismatch: " + record["source_cell"])
        record["source_row"] = row
        record["source_text"] = cells[record["source_cell"]]
        record["reply_source_cell"] = "D" + str(row)
        record["reply_template"] = cells[record["reply_source_cell"]]
        reply = record["reply_template"]
        if record["command"] == "TEMP_SET":
            reply = reply.replace("26度", str(record["value"]) + "度")
        elif record["command"] == "TIMER_OFF":
            reply = reply.replace("一小時", NUMBER_WORDS[record["value"]] + "小時")
        record["expected_reply_text"] = reply
        if "nanoe" in record["text"]:
            record["spoken_text"] = record["text"].replace("nanoe", "奈諾伊").replace(" ", "")
    profile = json.loads(REPLY_PROFILE.read_text(encoding="utf-8-sig"))
    for record in records:
        if record["command"] in profile["reply_overrides"]:
            record["expected_reply_text"] = profile["reply_overrides"][record["command"]]
            record["reply_override_source"] = "AudioRes/ReplyPhrases/ReplyProfile.json"
    return records


def validate(records):
    expected = {("Chinese(TWN)", "wake"): 3, ("Chinese(TWN)", "control"): 81,
                ("Taiwanese", "wake"): 2, ("Taiwanese", "control"): 51}
    for (language, stage), count in expected.items():
        selected = [r for r in records if (r["language"], r["stage"]) == (language, stage)]
        if len(selected) != count or len({r["text"] for r in selected}) != count:
            raise ValueError("Unexpected count or duplicate command: " + language + "/" + stage)
    for language, counts in (("Chinese(TWN)", (3, 6, 8, 21, 7, 4, 4, 6, 25)),
                             ("Taiwanese", (2, 5, 4, 19, 3, 2, 2, 3, 13))):
        for (group, _, _), count in zip(GROUPS, counts):
            if sum(r["language"] == language and r["group"] == group for r in records) != count:
                raise ValueError("Unexpected category count: " + language + "/" + group)
    meanings = {}
    for record in records:
        if not 0 < record["map_id"] < 32768 or not record["text"]:
            raise ValueError("Invalid command or MapID")
        key = (record["command"], record["value"], record["unit"])
        previous = meanings.setdefault(record["map_id"], key)
        if previous != key:
            raise ValueError("A MapID maps to conflicting actions")
    if len(records) != 137 or len(meanings) != 49:
        raise ValueError("Unexpected overall command or MapID count")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workbook", type=Path, default=DEFAULT_WORKBOOK)
    parser.add_argument("--replace-generated", action="store_true")
    args = parser.parse_args()
    records = build_records(read_cells(args.workbook))
    validate(records)
    output = Path(__file__).resolve().parent / "Input"
    output.mkdir(parents=True, exist_ok=True)
    payloads = {}
    for filename, language, stage in (
        ("WakeZHTW.txt", "Chinese(TWN)", "wake"),
        ("CommandsZHTW.txt", "Chinese(TWN)", "control"),
        ("WakeTW.txt", "Taiwanese", "wake"),
        ("CommandsTW.txt", "Taiwanese", "control"),
    ):
        lines = [r["text"] for r in records if (r["language"], r["stage"]) == (language, stage)]
        payloads[filename] = b"\xff\xfe" + ("\r\n".join(lines) + "\r\n").encode("utf-16le")
    for suffix, language in (("ZHTW", "Chinese(TWN)"), ("TW", "Taiwanese")):
        for label, pending in (("Core", False), ("PendingNanoe", True)):
            lines = [r["text"] for r in records
                     if r["language"] == language and r["stage"] == "control"
                     and ("nanoe" in r["text"]) == pending]
            payloads["Commands" + suffix + "." + label + ".txt"] = (
                b"\xff\xfe" + ("\r\n".join(lines) + "\r\n").encode("utf-16le"))
    for language, folder in (("Chinese(TWN)", "ChineseTWN"), ("Taiwanese", "Taiwanese")):
        lines = [r["text"] for r in records if r["language"] == language and "nanoe" not in r["text"]]
        payloads[folder + "/All.Core.txt"] = (
            b"\xff\xfe" + ("\r\n".join(lines) + "\r\n").encode("utf-16le"))
        for group, _, _ in GROUPS:
            lines = [r["text"] for r in records if r["language"] == language and r["group"] == group]
            payloads[folder + "/" + group + ".txt"] = (
                b"\xff\xfe" + ("\r\n".join(lines) + "\r\n").encode("utf-16le"))
    workbook = args.workbook.resolve()
    try:
        workbook_reference = workbook.relative_to(ROOT).as_posix()
    except ValueError:
        workbook_reference = workbook.name
    manifest = {"schema_version": 3, "source_workbook": workbook_reference,
                "source_worksheet": "完整語音指令與回覆",
                "source_sha256": hashlib.sha256(args.workbook.read_bytes()).hexdigest(),
                "requested_settings": {"sample_rate": 16000, "feature_dimension": 23,
                                       "frame_rate": 100, "model_size_level": 1},
                "nanoe_reading": {"text": "奈諾伊", "latin": "nano-E",
                                  "source": "User confirmed 2026-11-06"},
                "notes": ["Two hours uses 兩; additional 二 variants are not included",
                          "Phonemes and compiled models must be generated and verified by DSMT"],
                "reply_profile": "AudioRes/ReplyPhrases/ReplyProfile.json",
                "reply_profile_sha256": hashlib.sha256(REPLY_PROFILE.read_bytes()).hexdigest(),
                "records": records}
    payloads["CommandManifest.json"] = (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    lines = ["language\tcategory\tgroup\ttext\tMapID\tcommand\tvalue\tsource_cell"]
    lines += ["\t".join(str(r[k]) if r[k] is not None else "" for k in
                         ("language", "category", "group", "text", "map_id", "command", "value", "source_cell"))
              for r in records]
    payloads["MapID_Reference.txt"] = ("\r\n".join(lines) + "\r\n").encode("utf-8-sig")
    for name, payload in payloads.items():
        target = output / name
        if target.exists() and target.read_bytes() != payload and not args.replace_generated:
            raise FileExistsError(str(target) + " differs; review before --replace-generated")
    for name, payload in payloads.items():
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
    print("Generated 137 commands / 49 MapIDs in nine categories")
    print("Full wake/control counts: Chinese(TWN) 3/81; Taiwanese 2/51")
    print("Additional core/pending-nanoe inputs: Chinese(TWN) 77/4; Taiwanese 49/2")


if __name__ == "__main__":
    main()
