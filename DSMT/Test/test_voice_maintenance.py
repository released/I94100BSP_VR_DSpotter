"""Check the Windows maintenance entry points with Python absent from PATH."""

import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
TEST = ROOT / "DSMT/Test"


def main():
    fixture = Path(tempfile.mkdtemp(prefix="VoiceMaintenanceFixture_", dir=TEST)).resolve()
    marker = fixture / "OWNED_BY_VOICE_MAINTENANCE_TEST"
    marker.write_text("Owned fixture for no-Python maintenance verification.\n", encoding="ascii")
    project = fixture / "portable \u6e2c\u8a66"
    results = []
    transcript = []
    try:
        for directory in ("DSMT/Input", "DSMT/Combine/Working", "AudioRes/ReplyPhrases", "CMDSet/ReleaseV005"):
            shutil.copytree(ROOT / directory, project / directory)
        for name in ("DSMT/VoiceMaintenance.ps1", "DSMT/UpdateVoiceInputs.ps1", "DSMT/UpdateVoiceDocs.ps1", "DNN_VR/AirconVR.h"):
            path = project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, path)

        def run(name):
            env = os.environ.copy()
            env["PATH"] = ""
            powershell = str(Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe')
            command = [powershell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(project / 'DSMT' / name.replace('.cmd', '.ps1')), '-NoPause']
            result = subprocess.run(command, cwd=fixture, env=env, stdin=subprocess.DEVNULL,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
            output = result.stdout.decode("utf-8", errors="replace")
            transcript.append(name + "\n" + output)
            return result.returncode, output

        def passed(name):
            results.append({"case": name, "result": "PASS"})
            print("PASS " + name, flush=True)

        before = {p.relative_to(project).as_posix(): p.read_bytes() for p in (project / "DSMT/Input").rglob("*") if p.is_file()}
        book = project / "DSMT/Input/Panasonic_AirCon_Full_VoiceCommands.xlsx"
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                      wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
        kernel.CreateFileW.restype = wintypes.HANDLE
        kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        handle = kernel.CreateFileW(str(book), 0x40000000, 3, None, 3, 0, None)
        assert handle != ctypes.c_void_p(-1).value
        try:
            code, output = run("UpdateVoiceInputs.cmd")
        finally:
            kernel.CloseHandle(handle)
        assert code == 0, output
        actual = json.loads((project / "DSMT/Input/CommandManifest.json").read_text(encoding="utf-8-sig"))
        expected = json.loads(before["DSMT/Input/CommandManifest.json"])
        expected["reply_profile_sha256"] = hashlib.sha256((project / "AudioRes/ReplyPhrases/ReplyProfile.json").read_bytes()).hexdigest()
        expected["recognition_source"] = expected["source_workbook"]
        expected.pop("voice_ini_sha256", None)
        for record in expected["records"]:
            record.pop("ini_section", None)
            record.pop("ini_numbers", None)
        assert actual == expected, [(a['text'], {k: (a.get(k), e.get(k)) for k in set(a) | set(e) if a.get(k) != e.get(k)}) for a, e in zip(actual['records'], expected['records']) if a != e][:3]
        assert all((project / n).read_bytes() == b for n, b in before.items() if not n.endswith("CommandManifest.json"))
        passed("Input sync works with empty PATH, Unicode path and unrelated cwd; manifest semantics and text exports match")
        passed("An Excel-style shared writer handle permits a consistent read-only workbook snapshot")

        backups = set((project / "_Archive").glob("*.zip"))
        code, output = run("UpdateVoiceInputs.cmd")
        assert code == 0 and set((project / "_Archive").glob("*.zip")) == backups, output
        passed("Unchanged input sync is stable and creates no redundant backup")

        code, output = run("UpdateVoiceDocs.cmd")
        assert code == 0, output
        guide = (project / "DSMT/Test/UserCommandTestGuide.txt").read_text(encoding="utf-8-sig")
        roman = (project / "DSMT/Test/TaiwanesePronunciation.md").read_text(encoding="utf-8")
        quick = (project / "DSMT/Test/CommandMapIDQuickReference.md").read_text(encoding="utf-8")
        assert len(re.findall(r"^\[Z\d{3}\]", guide, re.M)) == 84
        assert len(re.findall(r"^\[T\d{3}\]", guide, re.M)) == 53
        assert len(re.findall(r"^\[M\d{3}\]", guide, re.M)) == 660
        assert len(re.findall(r"^\[F\d{2}\]", guide, re.M)) == 13
        assert len(re.findall(r"^\| \d+ \|", quick, re.M)) == 49
        native = json.loads((project / "DSMT/Combine/Working/Taiwanese/Taiwanese.dsmt").read_text(encoding="utf-8-sig"))
        words = [w for group in native["groups"] for entries in group.values() for w in entries]
        assert len(words) == 215
        assert all(w["phoneme"] in guide and w["phoneme"] in roman and w["phoneme"] in quick for w in words)
        passed("Documents cover 137 phrases, 660 pairs, 13 workflows, 49 MapIDs and all 215 Taiwanese pronunciation entries")

        notes = project / "DSMT/Test/UserCommandTestGuide.txt"
        annotated = notes.read_bytes() + b"\r\nTESTER_NOTES_TO_KEEP\r\n"
        notes.write_bytes(annotated)
        backups = set((project / "_Archive").glob("*.zip"))
        code, output = run("UpdateVoiceDocs.cmd")
        assert code == 0, output
        new_backup = (set((project / "_Archive").glob("*.zip")) - backups).pop()
        with zipfile.ZipFile(new_backup) as archive:
            assert archive.read("DSMT/Test/UserCommandTestGuide.txt") == annotated
        passed("Regenerating test sheets preserves filled notes in a verified backup")

        workbook = project / "DSMT/Input/Panasonic_AirCon_Full_VoiceCommands.xlsx"
        original_book = workbook.read_bytes()
        with zipfile.ZipFile(workbook) as archive:
            entries = {name: archive.read(name) for name in archive.namelist()}
        ns = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
        sheet = ET.fromstring(entries["xl/worksheets/sheet1.xml"])
        cell = sheet.find(".//m:c[@r='B3']", ns)
        assert cell is not None
        for child in list(cell):
            cell.remove(child)
        cell.set("t", "inlineStr")
        inline = ET.SubElement(cell, "{" + ns["m"] + "}is")
        ET.SubElement(inline, "{" + ns["m"] + "}t").text = "\u958b\u6a5f / \u958b\u51b7\u6c23 / \u958b\u7a7a\u8abf / \u958b\u555f\u51b7\u6c23"
        entries["xl/worksheets/sheet1.xml"] = ET.tostring(sheet, encoding="utf-8", xml_declaration=True)
        with zipfile.ZipFile(workbook, "w", zipfile.ZIP_DEFLATED) as archive:
            for name, data in entries.items():
                archive.writestr(name, data)
        manifest_path = project / "DSMT/Input/CommandManifest.json"
        baseline = manifest_path.read_bytes()
        code, output = run("UpdateVoiceInputs.cmd")
        assert code == 0, output
        changed = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
        alias = [r for r in changed["records"] if r["text"] == "\u958b\u555f\u51b7\u6c23"]
        assert len(changed["records"]) == 138 and len(alias) == 1
        assert alias[0]["map_id"] == 1 and alias[0]["command"] == "POWER_ON"
        passed("A new Excel alias inherits the existing action and MapID without editing hard-coded counts")

        document_snapshot = {p.name: p.read_bytes() for p in (project / "DSMT/Test").iterdir() if p.is_file()}
        code, output = run("UpdateVoiceDocs.cmd")
        assert code != 0 and "no native pronunciation" in output, output
        assert all((project / "DSMT/Test" / n).read_bytes() == b for n, b in document_snapshot.items())
        passed("An alias missing its native DSMT model is rejected before replacing documents")

        lock = project / ".voice-maintenance.lock"
        lock.write_text("ANOTHER_OWNER", encoding="ascii")
        workbook.write_bytes(original_book)
        current = manifest_path.read_bytes()
        code, output = run("UpdateVoiceInputs.cmd")
        assert code != 0 and lock.read_text() == "ANOTHER_OWNER" and manifest_path.read_bytes() == current
        lock.unlink()
        passed("An existing maintenance lock is preserved and output is not changed")

        manifest_path.write_bytes(baseline)
        (TEST / "VoiceMaintenanceTests.json").write_text(json.dumps({"result": "PASS", "checks": len(results), "cases": results}, indent=2) + "\n", encoding="utf-8")
    finally:
        (TEST / "VoiceMaintenanceTests.log").write_text("\n".join(transcript), encoding="utf-8-sig")
        assert fixture.parent == TEST.resolve() and marker.is_file()
        for directory, dirs, files in os.walk(fixture, followlinks=False):
            for name in dirs + files:
                assert not (Path(directory, name).lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT)
        shutil.rmtree(fixture)
    print("PASS owned fixture removed", flush=True)


if __name__ == "__main__":
    main()
