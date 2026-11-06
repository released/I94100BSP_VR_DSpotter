"""Move genuine native DSMT words into Excel categories without inventing phonemes."""

import argparse
import copy
import datetime
import hashlib
import json
from pathlib import Path
import zipfile

from build_command_inputs import GROUPS


def regroup(project_path, apply=False):
    root = Path(__file__).resolve().parent
    project_path = project_path.resolve()
    if not project_path.is_relative_to(root / "Combine/Working"):
        raise ValueError("Only projects inside DSMT/Combine/Working may be regrouped")
    original = project_path.read_bytes()
    project = json.loads(original.decode("utf-8-sig"))
    if project.get("ver") != "2.2":
        raise ValueError("Expected a native DSMT ver 2.2 project")
    records = json.loads((root / "Input/CommandManifest.json").read_text(encoding="utf-8"))["records"]
    records = [r for r in records if r["language"] == project["language"]]
    if not records:
        raise ValueError("Unsupported language")
    by_text = {r["text"]: r for r in records}
    order = {r["text"]: i for i, r in enumerate(records)}
    words = {name: [] for name, _, _ in GROUPS}
    origins = {}
    for old_group in project["groups"]:
        for old_name, old_words in old_group.items():
            for old_word in old_words:
                record = by_text.get(old_word["command"])
                if record is None:
                    raise ValueError("Unknown word; preserve and review: " + old_word["command"])
                if not all(old_word.get(k) for k in ("phoneme", "phoneme_id", "ipa_encode")):
                    raise ValueError("Native pronunciations missing: " + old_word["command"])
                word = copy.deepcopy(old_word)
                word["CmdMapID"] = record["map_id"]
                words[record["group"]].append(word)
                origins.setdefault(record["group"], old_name)
    new_groups = []
    for name, _, _ in GROUPS:
        words[name].sort(key=lambda w: order[w["command"]])
        for index, word in enumerate(words[name]):
            word["user_index"] = index
        if words[name]:
            new_groups.append({name: words[name]})
    if not new_groups:
        raise ValueError("Generate native pronunciations before regrouping an empty project")
    active_names = [next(iter(group)) for group in new_groups]
    settings = copy.deepcopy(project["setting"])
    for kind in ("SI", "global"):
        old = settings[kind]
        fallback = old.get("Group_2", next(iter(old.values())))
        settings[kind] = {
            name: copy.deepcopy(old.get(name, old.get(origins.get(name), fallback)))
            for name in active_names
        }
    changed = (project["groups"] != new_groups or project["setting"] != settings
               or project["platformFormat"]["nByteAlignment"] != "4")
    counts = {name: len(words[name]) for name, _, _ in GROUPS}
    seen = {w["command"] for entries in words.values() for w in entries}
    missing = [r for r in records if r["text"] not in seen]
    print(project_path.name, project["language"], counts, "missing", len(missing))
    if not changed or not apply:
        print("No changes required" if not changed else "Dry run; use --apply to write")
        return
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    archive_directory = root.parent / "_Archive"
    archive_directory.mkdir(parents=True, exist_ok=True)
    archive = archive_directory / ("Regroup_" + project_path.parent.name + "_" + stamp + ".zip")
    files = [p for p in project_path.parent.iterdir() if p.is_file()]
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as backup:
        for path in files:
            backup.write(path, path.name)
    with zipfile.ZipFile(archive) as backup:
        if backup.testzip() is not None or backup.read(project_path.name) != original:
            raise ValueError("Backup verification failed")
        stale = [p for p in files if
                 (p.name.startswith("Group_") and p.suffix in (".txt", ".mod"))
                 or p.name.endswith("_pack.bin") or p.suffix == ".zip"]
        for path in stale:
            if path.read_bytes() != backup.read(path.name):
                raise ValueError("Concurrent change; preserve: " + str(path))
    project["groups"] = new_groups
    project["setting"] = settings
    project["platformFormat"]["nByteAlignment"] = "4"
    project["bIsModify"] = True
    payload = json.dumps(project, ensure_ascii=False, separators=(",", ":")).encode("utf-8-sig")
    if project_path.read_bytes() != original:
        raise ValueError("Project changed during preparation; preserve it")
    project_path.write_bytes(payload)
    for path in stale:
        path.unlink()
    status = {
        "stage": "category_groups_prepared_requires_dsmt_save",
        "language": project["language"], "source_backup": str(archive.relative_to(root.parent)),
        "source_backup_path_base": "project_root",
        "project_sha256": hashlib.sha256(payload).hexdigest(),
        "model_entries": counts,
        "active_groups": active_names,
        "pending_groups": [name for name, _, _ in GROUPS if not words[name]],
        "unique_command_texts": {name: len({w["command"] for w in entries})
                                 for name, entries in words.items()},
        "remaining_count": len(missing), "remaining_records": missing,
        "dsmt_application_load_verified_after_regrouping": False,
        "compiled_models_generated_for_this_copy": False,
        "notes": ["Native pronunciation fields preserved verbatim",
                  "DSMT rejects empty groups; Health remains pending until nanoe pronunciations are confirmed",
                  "Old group and pack files retired to backup; save in DSMT before deployment"],
    }
    (project_path.parent / "ProjectStatus.json").write_text(
        json.dumps(status, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Prepared category groups; retained backup:", archive)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    regroup(args.project, args.apply)


if __name__ == "__main__":
    main()
