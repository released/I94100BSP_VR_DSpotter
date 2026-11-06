"""Validate native category models and package two CYBase instances for I94124."""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zlib
from build_reply_audio import AUDIO_ADDRESS, build_audio_image

ROOT = Path(__file__).resolve().parent.parent
MODEL_ADDRESS = 0x15000
FLASH_END = 0x80000
MAGIC = 0x31424341
LANGUAGES = ((1, "Chinese(TWN)", "ChineseTWN", "PanasonicAirConZHTW.dsmt"),
             (2, "Taiwanese", "Taiwanese", "Taiwanese.dsmt"))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load_firmware():
    path = ROOT / "Prj/KEIL/obj/DNN_NuOne_UAC.bin"
    if path.exists():
        return path.read_bytes()
    release = ROOT / "CMDSet/ReleaseV005"
    manifest = json.loads((release / "ModelManifest.json").read_text(encoding="utf-8-sig"))
    entry = manifest["files"]["DNN_NuOne_UAC.bin"]
    data = (release / "DNN_NuOne_UAC.bin").read_bytes()
    if len(data) != entry["bytes"] or sha(data) != entry["sha256"]:
        raise ValueError("Published firmware does not match ModelManifest.json")
    return data


def load_native(language, folder, filename, manifest):
    directory = ROOT / "DSMT/Combine/Working" / folder
    project_path = directory / filename
    project_data = project_path.read_bytes()
    project = json.loads(project_data.decode("utf-8-sig"))
    if project["language"] != language or project["platformFormat"]["nByteAlignment"] != "4":
        raise ValueError("Language/alignment mismatch: " + filename)
    lookup = {(r["group"], r["text"]): r for r in manifest if r["language"] == language}
    files = {"CYBase.mod": (directory / "CYBase.mod").read_bytes()}
    seen = set()
    group_summary = []
    for group in project["groups"]:
        name, words = next(iter(group.items()))
        if not words:
            raise ValueError("Empty groups must remain pending: " + name)
        expected_lines = Counter()
        for word in words:
            record = lookup[(name, word["command"])]
            if word["CmdMapID"] != record["map_id"] or not word["phoneme_id"]:
                raise ValueError("MapID/pronunciation mismatch: " + word["command"])
            expected_lines[(word["show_command"], word["phoneme"], word["CmdMapID"])] += 1
            seen.add((name, word["command"]))
        text = (directory / (name + ".txt")).read_text(encoding="utf-16")
        lines = [line for line in text.splitlines() if line.strip()]
        settings = ET.fromstring(lines[0])
        if settings.findtext("language") != language or settings.findtext("groupName") != name:
            raise ValueError("Exported group settings mismatch: " + name)
        actual_lines = Counter()
        for line in lines[1:]:
            fields = line.split("\t")
            actual_lines[(fields[0], fields[fields.index("-pr") + 1],
                          int(fields[fields.index("-mapid") + 1]))] += 1
        if actual_lines != expected_lines:
            raise ValueError("Saved text does not match the project: " + name)
        files[name + ".mod"] = (directory / (name + ".mod")).read_bytes()
        group_summary.append({"group": name, "entries": len(words),
                              "unique_texts": len({w["command"] for w in words})})
    missing = [r for key, r in lookup.items() if key not in seen]
    if any("nanoe" not in r["text"] for r in missing):
        raise ValueError("Non-nanoe commands still missing: " + language)
    chunks = list(files.values())
    if any(len(data) < 4 or len(data) % 4 for data in chunks):
        raise ValueError("Invalid model size/alignment: " + language)
    native = struct.pack("<I", len(chunks)) + struct.pack("<" + "I" * len(chunks),
                                                         *(len(data) for data in chunks))
    native += b"".join(chunks)
    native_path = directory / (project["projectName"] + "_pack.bin")
    if native_path.read_bytes() != native:
        raise ValueError("Native pack components mismatch: " + str(native_path))
    summary = {"language": language, "project": str(project_path.relative_to(ROOT)),
               "project_sha256": sha(project_data), "base_release": project["modelReleaseDate"],
               "groups": group_summary, "unique_texts": len(seen), "missing_records": missing,
               "native_pack_bytes": len(native), "native_pack_sha256": sha(native),
               "model_sha256": {name: sha(data) for name, data in files.items()},
               "human_pronunciation_review_complete": False}
    return native, summary


def build_bundle(packs):
    offset = 16 + len(packs) * 16
    entries = []
    payloads = []
    for language_id, data in packs:
        entries.append(struct.pack("<IIII", language_id, offset, len(data), zlib.crc32(data)))
        payloads.append(data)
        offset += len(data)
    return struct.pack("<IIII", MAGIC, 1, offset, len(packs)) + b"".join(entries + payloads)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "CMDSet/ReleaseV005")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "DSMT/Input/CommandManifest.json").read_text(encoding="utf-8"))["records"]
    packs = []
    summary = []
    payloads = {}
    for language_id, language, folder, filename in LANGUAGES:
        data, info = load_native(language, folder, filename, manifest)
        packs.append((language_id, data))
        summary.append(info)
        payloads[folder + ".pack.bin"] = data
    bundle = build_bundle(packs)
    if len(bundle) > AUDIO_ADDRESS - MODEL_ADDRESS:
        raise ValueError("Dual-language models overlap the internal audio region")
    audio, audio_info = build_audio_image(ROOT)
    firmware = load_firmware()
    if len(firmware) > MODEL_ADDRESS:
        raise ValueError("Firmware overlaps the model region")
    if b"[APP] release=V005 feedback=full_reply_led" not in firmware:
        raise ValueError("V005 firmware with internal audio support is required")
    payloads["AirconModels.bundle.bin"] = bundle
    payloads["DNN_NuOne_UAC.bin"] = firmware
    payloads["AirconReplies.bin"] = audio
    data_flash = bytearray(b"\xff" * (AUDIO_ADDRESS - MODEL_ADDRESS + len(audio)))
    data_flash[:len(bundle)] = bundle
    data_flash[AUDIO_ADDRESS - MODEL_ADDRESS:] = audio
    payloads["AirconDataFlash.bin"] = bytes(data_flash)
    record = {"format": "ACB1", "format_version": 1, "languages": summary,
              "release": "V005", "audio": audio_info,
              "programming": {"DNN_NuOne_UAC.bin": "0x00000000",
                              "AirconDataFlash.bin": "0x00015000"},
              "component_addresses": {"AirconModels.bundle.bin": "0x00015000",
                                      "AirconReplies.bin": "0x00070000"},
              "model_capacity_end_exclusive": "0x00070000",
              "model_end_exclusive": "0x%08X" % (MODEL_ADDRESS + len(bundle)),
              "flash_end_exclusive": "0x00080000",
              "files": {name: {"bytes": len(data), "sha256": sha(data)}
                        for name, data in payloads.items()},
              "verification_limits": ["Acoustic accuracy and real-time processing not board-tested",
                                      "Actual dual-engine RAM is checked and printed at runtime",
                                      "%d command text variants remain pending valid native pronunciations; nanoe reading confirmed by user"
                                      % sum(len(info["missing_records"]) for info in summary),
                                      "Existing binary ICP project was not rewritten"]}
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    old_manifest = output / "ModelManifest.json"
    old_files = json.loads(old_manifest.read_text(encoding="utf-8"))["files"] if old_manifest.exists() else {}
    for name, data in payloads.items():
        path = output / name
        if path.exists() and path.read_bytes() != data:
            if name not in old_files or sha(path.read_bytes()) != old_files[name]["sha256"]:
                raise FileExistsError("Preserve changed output: " + str(path))
    for name, data in payloads.items():
        (output / name).write_bytes(data)
    old_manifest.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Built ACB1 bilingual bundle:", len(bundle), "bytes; model end", record["model_end_exclusive"])
    print("Firmware:", len(firmware), "bytes; output", output)


if __name__ == "__main__":
    main()
