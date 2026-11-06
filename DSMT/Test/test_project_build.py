"""Exercise the one-click Windows pipeline with a structural audio fixture."""

import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TEST = ROOT / "DSMT/Test"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    fixture = Path(tempfile.mkdtemp(prefix="ProjectBuildFixture_", dir=TEST)).resolve()
    marker = fixture / "OWNED_BY_PROJECT_BUILD_TEST"
    marker.write_text("Owned structural pipeline fixture; acoustic quality is not tested.\n", encoding="ascii")
    project = fixture / "project \u6e2c\u8a66 \u7a7a\u767d"
    cases = []
    transcript = []
    try:
        for name in ("DSMT/Input", "DSMT/Combine/Working", "AudioRes/ReplyPhrases", "CMDSet/ReleaseV005"):
            shutil.copytree(ROOT / name, project / name)
        for name in ("BuildLatestModel.cmd", "DSMT/AirconBundleBuilder.exe", "DSMT/BuildProject.ps1",
                     "DSMT/ReplyAudioBuild.ps1", "DSMT/VoiceMaintenance.ps1", "DSMT/VoiceIni.ps1", "DSMT/VoiceCommands.ini", "DSMT/SyncNativeInputs.ps1",
                     "DSMT/UpdateVoiceDocs.ps1", "DSMT/WriteBuildReport.ps1", "DNN_VR/AirconVR.h"):
            p = project / name
            p.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, p)
        # Model/codec bytes come from the project. This seal defines a synthetic
        # provenance baseline for testing the pipeline, not a new audio encode.
        seal_path = project / "AudioRes/ReplyPhrases/AudioBuildManifest.json"
        seal = json.loads(seal_path.read_text(encoding="utf-8-sig"))
        seal["sources"] = {name: sha(project / name) for name in seal["sources"]}
        seal_path.write_text(json.dumps(seal), encoding="utf-8")
        binaries = ("DNN_NuOne_UAC.bin", "ChineseTWN.pack.bin", "Taiwanese.pack.bin",
                    "AirconModels.bundle.bin", "AirconReplies.bin", "AirconDataFlash.bin")
        release = project / "CMDSet/ReleaseV005"
        env = os.environ.copy()
        env["PATH"] = ""

        def run():
            command = '"' + env["COMSPEC"] + '" /d /s /c ""' + str(project / "BuildLatestModel.cmd") + '" --no-pause"'
            r = subprocess.run(command, cwd=fixture, env=env, stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
            text = r.stdout.decode("utf-8", errors="replace")
            transcript.append(text)
            if r.returncode:
                logs = sorted((project / 'DSMT/Test').glob('ProjectBuild_*.log'))
                if logs:
                    transcript.append(logs[-1].read_text(encoding='utf-8-sig'))
            return r.returncode, text

        def passed(name):
            cases.append({"case": name, "result": "PASS"})
            print("PASS " + name, flush=True)

        def snapshot():
            return {p.relative_to(project).as_posix(): p.read_bytes()
                    for folder in ("CMDSet/ReleaseV005", "DSMT/Input", "DSMT/Test")
                    for p in (project / folder).rglob("*") if p.is_file()
                    and p.suffix not in (".log",) and "Build" not in p.relative_to(project).parts}

        code, output = run()
        assert code == 0, output
        published = json.loads((release / "ModelManifest.json").read_text(encoding="utf-8-sig"))
        assert all(sha(release / n) == published["files"][n]["sha256"] for n in binaries)
        assert (release / "BuildReport.md").exists() and (project / "DSMT/Test/TaiwanesePronunciation.md").exists()
        assert all(not p.exists() for p in (project / "CMDSet/ReleaseV005/.bundle-build.lock", project / ".voice-maintenance.lock"))
        assert not list((project / "DSMT/Test/Build").glob("OneClick_*"))
        passed("One entry completes native sync, audio checks, six binaries, reports and spoken guides with empty PATH")

        before = snapshot()
        backups = set((project / "_Archive").glob("*.zip"))
        code, output = run()
        assert code == 0 and snapshot() == before, output
        assert set((project / "_Archive").glob("*.zip")) == backups
        passed("Identical repeat is stable and creates no redundant publication backup")

        ini = project / "DSMT/VoiceCommands.ini"
        original_ini = ini.read_bytes()
        ini.write_bytes(original_ini.replace("\u958b\u6a5f|0|0|0|1|N|".encode(), "\u958b\u555f\u51b7\u6c23|0|0|0|1|N|".encode(), 1))
        before = snapshot()
        code, output = run()
        assert code != 0 and snapshot() == before and "ChineseTWN/Group_2_Power" in output, output
        imported = project / "DSMT/Import/ChineseTWN/Group_2_Power.BatchAdd.txt"
        assert imported.read_text(encoding="utf-16").startswith("\u958b\u555f\u51b7\u6c23\t-pr\t")
        passed("An INI edit creates the DSMT import package and blocks publication until the native project matches")
        ini.write_bytes(original_ini)

        native = project / "DSMT/Combine/Working/ChineseTWN/PanasonicAirConZHTW.dsmt"
        text = project / "DSMT/Combine/Working/ChineseTWN/Group_2_Power.txt"
        original_native, original_text = native.read_bytes(), text.read_bytes()
        data = json.loads(original_native.decode("utf-8-sig"))
        words = next(g["Group_2_Power"] for g in data["groups"] if "Group_2_Power" in g)
        old_name = words[0]["command"]
        new_name = "\u96fb\u6e90\u958b\u555f"
        ini.write_bytes(original_ini.replace((old_name + "|").encode(), (new_name + "|").encode(), 1))
        words[0]["command"] = words[0]["show_command"] = new_name
        native.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8-sig")
        lines = original_text.decode("utf-16").splitlines()
        fields = lines[1].split("\t")
        assert fields[0] == old_name
        fields[0] = new_name
        lines[1] = "\t".join(fields)
        text.write_bytes(b"\xff\xfe" + ("\r\n".join(lines) + "\r\n").encode("utf-16le"))
        code, output = run()
        assert code == 0, output
        records = json.loads((project / "DSMT/Input/CommandManifest.json").read_text(encoding="utf-8-sig"))["records"]
        changed = next(r for r in records if r["text"] == new_name)
        assert changed["map_id"] == 1 and changed["source_origin"] == "INI"
        assert not any(r["text"] == old_name for r in records)
        passed("A matching INI and DSMT display rename inherits its MapID and updates guides without Excel")

        before = snapshot()
        extra = dict(words[0]); extra["command"] = extra["show_command"] = "\u555f\u52d5\u96fb\u6e90"
        words.append(extra)
        native.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8-sig")
        code, output = run()
        assert code != 0 and snapshot() == before, output
        assert "INI" in output
        passed("Unlisted native words are rejected against the canonical INI before publication")
        words.pop()
        native.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8-sig")

        saved_ini = ini.read_bytes()
        old_row = (new_name + "|0|0|0|1|N|").encode()
        ini.write_bytes(saved_ini.replace(old_row, (new_name + "|1|0|0|1|N|").encode(), 1))
        words[0]["reward"] = 1
        native.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8-sig")
        before = snapshot()
        code, output = run()
        assert code != 0 and snapshot() == before and "\u6a21\u578b\u5c1a\u672a\u66f4\u65b0" in output, output
        passed("Matching INI and native parameter edits still require a regenerated model, even when the row count is unchanged")
        words[0]["reward"] = 0
        native.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8-sig")
        ini.write_bytes(saved_ini)

        wav = project / "AudioRes/ReplyPhrases/WavSrc/reply_temp_down.wav"
        original_wav = wav.read_bytes(); changed_wav = bytearray(original_wav); changed_wav[-2] ^= 1; wav.write_bytes(changed_wav)
        before = snapshot()
        code, output = run()
        assert code != 0 and snapshot() == before and "F7" in output, output
        passed("A replaced WAV without an Audio Tool encode gives one actionable F7 instruction and preserves output")
        wav.write_bytes(original_wav)

        rom = project / "AudioRes/ReplyPhrases/Output/AirconReplies_AudioInfoMerge.ROM"
        original_rom = rom.read_bytes(); broken = bytearray(original_rom)
        first = struct.unpack_from("<I", broken, 16)[0]
        struct.pack_into("<I", broken, first + 8, 640)
        rom.write_bytes(broken)
        before = snapshot()
        code, output = run()
        assert code != 0 and snapshot() == before, output
        passed("Automatic audio sealing retains codec validation; invalid bitrate never replaces published files")
        rom.write_bytes(original_rom)

        lock = release / ".bundle-build.lock"; lock.write_text("ANOTHER_OWNER", encoding="ascii")
        before = snapshot()
        code, output = run()
        assert code != 0 and snapshot() == before and lock.read_text() == "ANOTHER_OWNER", output
        lock.unlink()
        passed("An existing publisher lock is preserved and staged work is cleaned")

        profile = project / "AudioRes/ReplyPhrases/ReplyProfile.json"; original_profile = profile.read_bytes()
        changed = json.loads(original_profile.decode("utf-8-sig")); changed["phrases"][2]["text"] = "\u96fb\u6e90\u5df2\u958b\u555f"
        profile.write_text(json.dumps(changed, ensure_ascii=False), encoding="utf-8")
        before = snapshot()
        code, output = run()
        assert code != 0 and snapshot() == before and "WAV" in output, output
        passed("Noninteractive text edits stop before synthesis or GUI launch; interactive mode is explicitly guided")
        profile.write_bytes(original_profile)

        assert not list((project / "DSMT/Test/Build").glob("OneClick_*"))
        (TEST / "ProjectBuildTests.json").write_text(json.dumps({"result":"PASS", "checks":len(cases), "cases":cases,
            "limits":["Structural fixture only; no acoustic verification", "Interactive Audio Tool F7 requires an unlocked desktop"]}, indent=2) + "\n", encoding="utf-8")
    finally:
        (TEST / "ProjectBuildTests.log").write_text("\n".join(transcript), encoding="utf-8-sig")
        assert fixture.parent == TEST.resolve() and marker.is_file()
        for directory, dirs, files in os.walk(fixture, followlinks=False):
            for name in dirs + files:
                assert not (Path(directory, name).lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT)
        shutil.rmtree(fixture)
    print("PASS owned pipeline fixture removed", flush=True)


if __name__ == "__main__":
    main()
