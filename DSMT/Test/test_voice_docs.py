"""Check document generation from INI-derived records, without an Excel source."""

import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parents[2]
TEST = ROOT / "DSMT/Test"


def main():
    fixture = Path(tempfile.mkdtemp(prefix="VoiceMaintenanceFixture_", dir=TEST)).resolve()
    marker = fixture / "OWNED_BY_VOICE_DOCS_TEST"
    marker.write_text("Disposable voice document test project.\n", encoding="ascii")
    project = fixture / "project \u6e2c\u8a66"
    checks = 0

    def run():
        env = os.environ.copy()
        env["PATH"] = ""
        powershell = str(Path(os.environ["SystemRoot"]) / "System32/WindowsPowerShell/v1.0/powershell.exe")
        result = subprocess.run([powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                                 str(project / "DSMT/UpdateVoiceDocs.ps1"), "-NoPause"],
                                cwd=fixture, env=env, stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
        return result.returncode, result.stdout.decode("utf-8", errors="replace")

    def passed(name):
        nonlocal checks
        checks += 1
        print("PASS " + name, flush=True)

    def documents():
        return {p.name: p.read_bytes() for p in (project / "DSMT/Test").iterdir() if p.is_file()}

    try:
        names = ["DSMT/VoiceMaintenance.ps1", "DSMT/UpdateVoiceDocs.ps1", "DNN_VR/AirconVR.h",
                 "DSMT/Input/CommandManifest.json", "CMDSet/ReleaseV005/ModelManifest.json"]
        names += [p.relative_to(ROOT).as_posix() for p in (ROOT / "DSMT/Combine/Working").rglob("*.dsmt")]
        for name in names:
            target = project / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        assert not list(project.rglob("*.xlsx"))
        code, output = run()
        assert code == 0, output
        guide = (project / "DSMT/Test/UserCommandTestGuide.txt").read_text(encoding="utf-8-sig")
        roman = (project / "DSMT/Test/TaiwanesePronunciation.md").read_text(encoding="utf-8-sig")
        quick = (project / "DSMT/Test/CommandMapIDQuickReference.md").read_text(encoding="utf-8-sig")
        assert len(re.findall(r"^\[Z\d{3}\]", guide, re.M)) == 84
        assert len(re.findall(r"^\[T\d{3}\]", guide, re.M)) == 53
        assert len(re.findall(r"^\[M\d{3}\]", guide, re.M)) == 660
        assert len(re.findall(r"^\[F\d{2}\]", guide, re.M)) == 13
        assert len(re.findall(r"^\| \d+ \|", quick, re.M)) == 49
        assert "Excel" not in guide and "INI [ChineseTWN." in guide
        native = json.loads((project / "DSMT/Combine/Working/Taiwanese/Taiwanese.dsmt").read_text(encoding="utf-8-sig"))
        words = [word for group in native["groups"] for values in group.values() for word in values]
        assert len(words) == 215 and all(w["phoneme"] in guide and w["phoneme"] in roman and w["phoneme"] in quick for w in words)
        passed("Without Excel or Python PATH, documents cover all phrases, pairs, workflows, MapIDs and pronunciation candidates")

        before = documents()
        backups = set((project / "_Archive").glob("*.zip"))
        code, output = run()
        assert code == 0 and documents() == before and set((project / "_Archive").glob("*.zip")) == backups, output
        passed("Unchanged document generation is stable and creates no redundant backup")

        notes = project / "DSMT/Test/UserCommandTestGuide.txt"
        annotated = notes.read_bytes() + b"\r\nTESTER_NOTES_TO_KEEP\r\n"
        notes.write_bytes(annotated)
        code, output = run()
        assert code == 0, output
        new_backups = set((project / "_Archive").glob("*.zip")) - backups
        assert len(new_backups) == 1
        with zipfile.ZipFile(new_backups.pop()) as archive:
            assert archive.read("DSMT/Test/UserCommandTestGuide.txt") == annotated
        passed("Filled test notes remain in a verified backup before regeneration")

        manifest = project / "DSMT/Input/CommandManifest.json"
        original = manifest.read_bytes()
        data = json.loads(original.decode("utf-8-sig"))
        alias = dict(next(r for r in data["records"] if r["language"] == "Chinese(TWN)" and r["map_id"] == 1))
        alias["text"] = "\u555f\u52d5\u51b7\u6c23"
        data["records"].append(alias)
        manifest.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
        before = documents()
        code, output = run()
        assert code != 0 and "no native pronunciation" in output and documents() == before, output
        manifest.write_bytes(original)
        passed("A word missing from native DSMT pronunciation is rejected before document replacement")

        lock = project / ".voice-maintenance.lock"
        lock.write_text("ANOTHER_OWNER", encoding="ascii")
        notes.write_bytes(annotated)
        before = documents()
        code, output = run()
        assert code != 0 and lock.read_text() == "ANOTHER_OWNER" and documents() == before, output
        passed("An existing maintenance lock and its protected documents remain intact")
    finally:
        assert fixture.parent == TEST.resolve() and marker.is_file()
        for directory, dirs, files in os.walk(fixture, followlinks=False):
            for name in dirs + files:
                assert not (Path(directory, name).lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT)
        shutil.rmtree(fixture)
    print(f"PASS {checks} document checks; owned fixture removed", flush=True)


if __name__ == "__main__":
    main()
