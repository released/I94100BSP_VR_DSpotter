"""Validate sealed NuOne phrase resources and wrap the internal-flash audio image."""

import hashlib
import json
from pathlib import Path
import struct
import zlib

AUDIO_ADDRESS = 0x70000
FLASH_END = 0x80000
PREFIX = "AudioRes/ReplyPhrases/"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def build_audio_image(root):
    root = Path(root)
    seal = json.loads((root / PREFIX / "AudioBuildManifest.json").read_text(encoding="utf-8-sig"))
    if (seal["schema_version"], seal["sample_rate"], seal["clips"], seal["bits_per_sample"]) != (1, 16000, 23, 1):
        raise ValueError("Audio build settings mismatch")
    sources = seal["sources"]
    if len(sources) != 28:
        raise ValueError("Incomplete sealed audio sources")
    for relative, expected in sources.items():
        path = (root / relative).resolve()
        path.relative_to(root.resolve())
        if digest(path.read_bytes()) != expected:
            raise ValueError("Audio source changed; rebuild and seal: " + relative)
    profile_path = PREFIX + "ReplyProfile.json"
    source_path = PREFIX + "WavSrc/PhraseSourceManifest.json"
    header_path = PREFIX + "Output/AirconReplies_AudioInfo.h"
    rom_path = PREFIX + "Output/AirconReplies_AudioInfoMerge.ROM"
    for relative in (profile_path, source_path, header_path, rom_path, PREFIX + "WavSrc/AirconReplies.uid"):
        if relative not in sources:
            raise ValueError("Missing sealed audio input: " + relative)
    profile_bytes = (root / profile_path).read_bytes()
    profile = json.loads(profile_bytes.decode("utf-8-sig"))
    source = json.loads((root / source_path).read_text(encoding="utf-8-sig"))
    if source["source_profile_sha256"] != digest(profile_bytes):
        raise ValueError("Phrase source profile is stale")
    header = (root / header_path).read_text(encoding="utf-8-sig").replace(" ", "").replace("\t", "")
    if len(profile["phrases"]) != 23 or len(source["entries"]) != 23:
        raise ValueError("Expected 23 clips")
    seen = set()
    for index, (phrase, wave) in enumerate(zip(profile["phrases"], source["entries"])):
        file = phrase["file"]
        relative = PREFIX + "WavSrc/" + file
        if (Path(file).name != file or phrase["index"] != index or wave["index"] != index
                or wave["file"] != file or relative not in sources
                or wave["sha256"] != digest((root / relative).read_bytes())):
            raise ValueError("Audio clip order/source mismatch")
        if Path(file).stem.upper() + "=" + str(index) + "," not in header:
            raise ValueError("Generated audio ID mismatch")
        for map_id in phrase["map_ids"]:
            expected = (map_id + 1 if 1 <= map_id <= 19 else 21 if 116 <= map_id <= 130
                        else 22 if 201 <= map_id <= 212 else 0 if 1001 <= map_id <= 1003 else -1)
            if expected != index or map_id in seen:
                raise ValueError("Audio MapID table mismatch")
            seen.add(map_id)
    if len(seen) != 49:
        raise ValueError("Audio MapID coverage mismatch")
    rom = (root / rom_path).read_bytes()
    if len(rom) < 200 or len(rom) + 16 > FLASH_END - AUDIO_ADDRESS:
        raise ValueError("Audio capacity mismatch")
    if struct.unpack_from("<3I", rom) != (23, 0, 16):
        raise ValueError("Audio ROM header mismatch")
    cursor = 200
    for index in range(23):
        offset, size = struct.unpack_from("<2I", rom, 16 + index * 8)
        if offset != cursor or size < 12 or size > len(rom) - cursor:
            raise ValueError("Audio clip bounds mismatch")
        fmt, rate, payload, bits = struct.unpack_from("<HHII", rom, offset)
        if fmt != 0x40 or rate != 16000 or bits != 320 or not payload or payload % 40 or payload > size - 12:
            raise ValueError("Expected NuOne 16 kHz, 1.0 bits/sample")
        cursor += size
    if cursor != len(rom):
        raise ValueError("Audio ROM trailing bytes mismatch")
    image = struct.pack("<4I", 0x31524341, 1, len(rom), zlib.crc32(rom) & 0xFFFFFFFF) + rom
    image += b"\0" * (-len(image) % 4)
    info = {"format": "ACR1", "address": "0x00070000", "rom_bytes": len(rom),
            "rom_sha256": digest(rom), "clips": 23, "sample_rate": 16000, "bits_per_sample": 1,
            "profile": profile_path, "profile_sha256": digest(profile_bytes)}
    return image, info
