"""Developer-only regression checks for the Python-free Windows bundle builder."""

import contextlib
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parents[2]
TEST = ROOT / "DSMT/Test"
RESULTS = []
TRANSCRIPT = []
PAYLOADS = ("ChineseTWN.pack.bin", "Taiwanese.pack.bin", "AirconModels.bundle.bin", "DNN_NuOne_UAC.bin",
            "AirconReplies.bin", "AirconDataFlash.bin")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def snapshot(directory):
    return {p.name: p.read_bytes() for p in directory.iterdir()
            if p.is_file() and p.name != ".bundle-build.lock"}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_builder(project, args=(), wrapper=False):
    env = os.environ.copy()
    env["PATH"] = ""
    if wrapper:
        require(not any(c in str(project) for c in '&|<>^%"\r\n'), "Unsupported shell character in regression fixture path")
        command = '""' + str(project / "BuildLatestModel.cmd") + '" --no-pause'
        if args:
            command += " " + subprocess.list2cmdline(list(args))
        command += '"'
        argv = '"' + os.environ.get("COMSPEC", r"C:\Windows\System32\cmd.exe") + '" /d /s /c ' + command
    else:
        argv = [str(project / "DSMT/AirconBundleBuilder.exe"), "--no-pause"] + list(args)
    result = subprocess.run(argv, cwd=project.parent, env=env, stdin=subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
    output = result.stdout.decode("utf-8-sig", errors="replace")
    TRANSCRIPT.append("COMMAND: " + (argv if isinstance(argv, str) else subprocess.list2cmdline(argv)) + "\n" + output)
    return result.returncode, output


@contextlib.contextmanager
def edits(*paths):
    before = {p: p.read_bytes() if p.exists() else None for p in paths}
    try:
        yield
    finally:
        for path, data in before.items():
            if data is None:
                if path.exists():
                    path.unlink()
            else:
                path.write_bytes(data)


def write_json(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8-sig")


@contextlib.contextmanager
def locked_file(path, share):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID,
                                  wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    handle = kernel.CreateFileW(str(path), 0x80000000, share, None, 3, 0, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        yield
    finally:
        kernel.CloseHandle(handle)


def main():
    fixture = Path(tempfile.mkdtemp(prefix="BundleBuilderFixture_", dir=str(TEST))).resolve()
    marker = fixture / "OWNED_BY_TEST_BUNDLE_BUILDER"
    marker.write_text("Disposable regression data owned by this test.\n", encoding="ascii")
    project = fixture / "portable \u6e2c\u8a66 \u7a7a\u767d"
    project.mkdir()
    (project / "DSMT/Input").mkdir(parents=True)
    (project / "Prj/KEIL/obj").mkdir(parents=True)
    shutil.copyfile(ROOT / "BuildLatestModel.cmd", project / "BuildLatestModel.cmd")
    shutil.copyfile(ROOT / "DSMT/AirconBundleBuilder.exe", project / "DSMT/AirconBundleBuilder.exe")
    shutil.copyfile(ROOT / "DSMT/Input/CommandManifest.json", project / "DSMT/Input/CommandManifest.json")
    shutil.copytree(ROOT / "AudioRes/ReplyPhrases", project / "AudioRes/ReplyPhrases")
    for folder, filename in (("ChineseTWN", "PanasonicAirConZHTW.dsmt"), ("Taiwanese", "Taiwanese.dsmt")):
        source = ROOT / "DSMT/Combine/Working" / folder
        target = project / "DSMT/Combine/Working" / folder
        target.mkdir(parents=True)
        data = json.loads((source / filename).read_text(encoding="utf-8-sig"))
        names = [filename, "CYBase.mod", data["projectName"] + "_pack.bin"]
        for group in data["groups"]:
            name = next(iter(group))
            names.extend((name + ".mod", name + ".txt"))
        for name in names:
            shutil.copyfile(source / name, target / name)
    firmware = project / "Prj/KEIL/obj/DNN_NuOne_UAC.bin"
    firmware_source = ROOT / "Prj/KEIL/obj/DNN_NuOne_UAC.bin"
    if not firmware_source.exists():
        firmware_source = ROOT / "CMDSet/ReleaseV005/DNN_NuOne_UAC.bin"
    shutil.copyfile(firmware_source, firmware)
    release = project / "CMDSet/ReleaseV005"
    shutil.copytree(ROOT / "CMDSet/ReleaseV005", release)
    reference = fixture / "python_reference"
    result = subprocess.run([sys.executable, "-B", "-X", "utf8", str(ROOT / "DSMT/build_model_bundle.py"),
                             "--output", str(reference)], cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=30)
    require(result.returncode == 0, "Canonical Python reference failed")
    TRANSCRIPT.append("REFERENCE:\n" + result.stdout.decode("utf-8-sig"))

    def test(name, action):
        action()
        RESULTS.append({"name": name, "result": "PASS"})
        print("PASS " + name, flush=True)

    def success(args=(), wrapper=False):
        code, output = run_builder(project, args, wrapper)
        require(code == 0 and "SUCCESS:" in output, output)
        return output

    def failure(expected, args=()):
        before = snapshot(release)
        archives = set((project / "_Archive").glob("*.zip")) if (project / "_Archive").exists() else set()
        code, output = run_builder(project, args)
        require(code != 0 and expected in output, output)
        require(snapshot(release) == before, "Failed build changed release files")
        after = set((project / "_Archive").glob("*.zip")) if (project / "_Archive").exists() else set()
        require(after == archives, "Rejected validation unexpectedly archived/published output")

    def parity():
        success(wrapper=True)
        for name in PAYLOADS:
            require((release / name).read_bytes() == (reference / name).read_bytes(), "Binary differs: " + name)
        require(json.loads((release / "ModelManifest.json").read_text(encoding="utf-8-sig")) ==
                json.loads((reference / "ModelManifest.json").read_text(encoding="utf-8-sig")),
                "Manifest semantics differ from Python")

    test("Batch entry works with empty PATH, Unicode/spaces and unrelated working directory; six binaries and manifest match Python", parity)
    baseline = snapshot(release)

    def repeat():
        before = snapshot(release)
        archives = set((project / "_Archive").glob("*.zip"))
        output = success(wrapper=True)
        require("no release files changed" in output and snapshot(release) == before, "No-change build was not stable")
        require(set((project / "_Archive").glob("*.zip")) == archives, "No-change build created another backup")

    test("Repeated identical input is stable and creates no redundant backup", repeat)

    def fallback():
        with edits(firmware):
            firmware.unlink()
            output = success(("--output", "CMDSet/Fallback"))
            require("hash-verified compatible published" in output, output)
            require((project / "CMDSet/Fallback/DNN_NuOne_UAC.bin").read_bytes() == baseline["DNN_NuOne_UAC.bin"], "Firmware fallback differs")

    test("No Keil output: compatible published firmware fallback is hash-verified", fallback)

    def bad_fallback():
        with edits(firmware, release / "DNN_NuOne_UAC.bin"):
            firmware.unlink()
            (release / "DNN_NuOne_UAC.bin").write_bytes(b"tampered")
            failure("fallback hash mismatch", ("--output", "CMDSet/RejectedFallback"))
            require(not (project / "CMDSet/RejectedFallback").exists(), "Failed fallback created output")

    test("Tampered published firmware fallback is rejected without publication", bad_fallback)
    native = project / "DSMT/Combine/Working/Taiwanese"
    native_project = native / "Taiwanese.dsmt"

    def project_error(edit, expected):
        with edits(native_project):
            data = json.loads(native_project.read_text(encoding="utf-8-sig"))
            edit(data)
            write_json(native_project, data)
            failure(expected)

    test("Wrong native MapID is rejected", lambda: project_error(lambda d: d["groups"][1]["Group_2_Power"][0].update(CmdMapID=999), "MapID/native"))
    test("Fractional MapID is rejected", lambda: project_error(lambda d: d["groups"][1]["Group_2_Power"][0].update(CmdMapID=1.5), "must be an integer"))
    test("Unknown alias cannot bypass the requirements manifest", lambda: project_error(lambda d: d["groups"][1]["Group_2_Power"][0].update(command="unregistered alias"), "Unknown word/category"))
    test("Missing genuine native pronunciation ID is rejected", lambda: project_error(lambda d: d["groups"][1]["Group_2_Power"][0].update(phoneme_id=""), "MapID/native"))
    test("Wrong language/alignment is rejected", lambda: project_error(lambda d: d["platformFormat"].update(nByteAlignment="1"), "Language/alignment"))

    def wake_order(d):
        d["groups"][0], d["groups"][1] = d["groups"][1], d["groups"][0]

    test("Wake group must remain first for firmware stage routing", lambda: project_error(wake_order, "must be the first"))
    test("Empty groups are rejected", lambda: project_error(lambda d: d["groups"][1].update(Group_2_Power=[]), "Empty group"))

    def bad_export():
        path = native / "Group_2_Power.txt"
        with edits(path):
            lines = path.read_text(encoding="utf-16").splitlines()
            lines[1] = "bad display" + lines[1][lines[1].index("\t"):]
            path.write_bytes(b"\xff\xfe" + ("\r\n".join(lines) + "\r\n").encode("utf-16le"))
            failure("Saved text does not match")

    test("Native project/export pronunciation count mismatch is rejected", bad_export)

    def bad_pack():
        path = native / "Taiwanese_pack.bin"
        with edits(path):
            data = bytearray(path.read_bytes()); data[-1] ^= 1
            path.write_bytes(data)
            failure("Native pack components mismatch")

    test("Stale or corrupted native pack is rejected", bad_pack)

    def bad_firmware():
        with edits(firmware):
            firmware.write_bytes(b"\x00" * (0x15000 + 1))
            failure("overlaps the model region")

    test("Firmware/model flash overlap is rejected", bad_firmware)

    @contextlib.contextmanager
    def changed_model(extra):
        group = native / "Group_4_Temperature.mod"
        pack = native / "Taiwanese_pack.bin"
        with edits(group, pack):
            group.write_bytes(group.read_bytes() + b"\x00" * extra)
            data = json.loads(native_project.read_text(encoding="utf-8-sig"))
            chunks = [(native / "CYBase.mod").read_bytes()] + [(native / (next(iter(g)) + ".mod")).read_bytes() for g in data["groups"]]
            pack.write_bytes(struct.pack("<I", len(chunks)) + struct.pack("<" + "I" * len(chunks), *(len(c) for c in chunks)) + b"".join(chunks))
            yield

    def too_large():
        with changed_model(40000):
            failure("overlap the internal audio region")

    test("Combined dual-language model flash overflow is rejected", too_large)

    audio_dir = project / "AudioRes/ReplyPhrases"
    audio_seal = audio_dir / "AudioBuildManifest.json"

    def audio_error(relative, edit, expected, reseal=False):
        path = project / relative
        with edits(path, audio_seal):
            path.write_bytes(edit(path.read_bytes()))
            if reseal:
                seal = json.loads(audio_seal.read_text(encoding="utf-8-sig"))
                seal["sources"][relative] = sha(path.read_bytes())
                write_json(audio_seal, seal)
            failure(expected)

    rom_relative = "AudioRes/ReplyPhrases/Output/AirconReplies_AudioInfoMerge.ROM"
    test("Encoded audio modified after sealing is rejected", lambda: audio_error(
        rom_relative, lambda b: b[:-1] + bytes([b[-1] ^ 1]), "Audio source changed"))
    test("WAV modified without a new audio build is rejected", lambda: audio_error(
        "AudioRes/ReplyPhrases/WavSrc/reply_temp_set.wav", lambda b: b[:-1] + bytes([b[-1] ^ 1]), "Audio source changed"))
    test("Changed reply profile requires regenerated audio", lambda: audio_error(
        "AudioRes/ReplyPhrases/ReplyProfile.json", lambda b: b + b" ", "Audio source changed"))

    def wrong_bitrate(content):
        data = bytearray(content)
        offset = struct.unpack_from("<I", data, 16)[0]
        struct.pack_into("<I", data, offset + 8, 256)
        return bytes(data)

    test("Resealed audio with the wrong bitrate is rejected", lambda: audio_error(
        rom_relative, wrong_bitrate, "1.0 bits/sample", True))
    test("Audio region overflow is rejected", lambda: audio_error(
        rom_relative, lambda b: b + b"\0" * 65536, "header/capacity mismatch", True))
    test("Generated audio ID mismatch is rejected", lambda: audio_error(
        "AudioRes/ReplyPhrases/Output/AirconReplies_AudioInfo.h",
        lambda b: b.replace(b"REPLY_TEMP_SET=21", b"REPLY_TEMP_SET=22"), "audio ID header mismatch", True))

    def missing_seal():
        with edits(audio_seal):
            audio_seal.unlink()
            failure("AudioBuildManifest")

    test("Missing audio build manifest is rejected", missing_seal)

    def old_firmware():
        with edits(firmware):
            data = firmware.read_bytes()
            marker = b"[APP] release=V005 feedback=full_reply_led"
            require(data.count(marker) == 1, "Expected one V005 firmware marker")
            firmware.write_bytes(data.replace(marker, marker.replace(b"V005", b"V004")))
            failure("V005 firmware")

    test("Firmware without the V005 internal-audio marker is rejected", old_firmware)

    def tampered_output():
        path = release / "AirconModels.bundle.bin"
        with edits(path):
            data = bytearray(path.read_bytes()); data[-1] ^= 1
            path.write_bytes(data)
            failure("Preserve changed output")

    test("User-modified output is preserved rather than overwritten", tampered_output)

    def exclusive_builder():
        path = release / ".bundle-build.lock"
        path.write_bytes(b"")
        try:
            with locked_file(path, 0):
                before = snapshot(release)
                code, output = run_builder(project)
                require(code != 0 and "Another builder" in output, output)
                require(snapshot(release) == before, "Concurrent build changed output")
                require(path.exists(), "Another builder's lock was removed")
        finally:
            path.unlink()

    test("Concurrent publication is refused without deleting another builder's lock", exclusive_builder)

    def backup_and_update():
        with changed_model(4):
            before = snapshot(release)
            archives = set((project / "_Archive").glob("*.zip"))
            success()
            created = set((project / "_Archive").glob("*.zip")) - archives
            require(len(created) == 1, "Changed publication did not create one backup")
            with zipfile.ZipFile(created.pop()) as z:
                require(z.testzip() is None and set(z.namelist()) == set(before), "Backup file coverage mismatch")
                require(all(z.read(name) == content for name, content in before.items()), "Backup bytes differ")
            require((release / "AirconModels.bundle.bin").read_bytes() != before["AirconModels.bundle.bin"], "Changed model was not published")
        for name, content in baseline.items():
            (release / name).write_bytes(content)

    test("Changed inputs create a byte-verified rollback ZIP before publication", backup_and_update)

    def interrupted_publish():
        with changed_model(4):
            before = snapshot(release)
            with locked_file(release / "ModelManifest.json", 1):
                code, output = run_builder(project)
                require(code != 0 and "ERROR:" in output, output)
            require(snapshot(release) == before, "Late publication failure did not restore earlier replaced files")
            require(not list(release.glob(".bundle-stage-*")), "Owned staging directory left behind")

    test("Late locked-manifest failure rolls back earlier binary replacements", interrupted_publish)

    def output_escape():
        failure("Path must be inside", ("--output", str(fixture / "outside_output")))
        require(not (fixture / "outside_output").exists(), "Output escaped project CMDSet")

    test("Output cannot escape project CMDSet", output_escape)
    report = {
        "result": "PASS", "checks": len(RESULTS), "cases": RESULTS,
        "no_python_in_production_path": True, "runtime_environment": "Windows .NET Framework executable; PATH empty",
        "reference": "Existing build_model_bundle.py; same inputs, all four binaries byte-identical, ModelManifest structurally equal",
        "executable_sha256": sha((ROOT / "DSMT/AirconBundleBuilder.exe").read_bytes()),
        "source_sha256": sha((ROOT / "DSMT/AirconBundleBuilder.cs").read_bytes()),
        "canonical_payloads": {name: {"bytes": (reference / name).stat().st_size, "sha256": sha((reference / name).read_bytes())} for name in PAYLOADS},
        "fixture_cleanup": "Completed; owned fixture tree removed after path, marker and reparse checks"
    }
    require(fixture.parent == TEST.resolve() and fixture.name.startswith("BundleBuilderFixture_"), "Unexpected fixture location")
    require(marker.is_file(), "Fixture ownership marker missing")
    require(not fixture.is_symlink() and not fixture.is_junction(), "Fixture root is a reparse point")
    require(all(not p.is_symlink() and not p.is_junction() for p in fixture.rglob("*")), "Fixture contains a reparse point")
    shutil.rmtree(fixture)
    (TEST / "BundleBuilderTests.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (TEST / "BundleBuilderTests.log").write_text("\n".join(TRANSCRIPT), encoding="utf-8-sig")
    print("PASS %d portable packager regression cases; owned fixture removed" % len(RESULTS))


if __name__ == "__main__":
    try:
        main()
    except Exception:
        (TEST / "BundleBuilderTests.failure.log").write_text("\n".join(TRANSCRIPT), encoding="utf-8-sig")
        raise
