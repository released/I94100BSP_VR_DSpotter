"""Exercise the upload batch against disposable local repositories only."""

import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
TEST = ROOT / "DSMT/Test"
DESTINATION = "https://github.com/released/I94100BSP_VR_DSpotter.git"


def main():
    git_exe = shutil.which("git")
    assert git_exe, "Git for Windows is required"
    env = os.environ.copy()
    env.update({"GIT_CONFIG_NOSYSTEM": "1", "GIT_CONFIG_GLOBAL": os.devnull,
                "GIT_TERMINAL_PROMPT": "0", "GIT_AUTHOR_NAME": "Upload Fixture",
                "GIT_AUTHOR_EMAIL": "fixture@example.invalid", "GIT_COMMITTER_NAME": "Upload Fixture",
                "GIT_COMMITTER_EMAIL": "fixture@example.invalid"})
    env["PATH"] = os.pathsep.join([str(Path(git_exe).parent), str(Path(env["SYSTEMROOT"]) / "System32"), env["SYSTEMROOT"]])
    fixture = Path(tempfile.mkdtemp(prefix="GitUploadFixture_", dir=TEST)).resolve()
    marker = fixture / "OWNED_BY_GITHUB_PUSH_TEST"
    marker.write_text("Disposable Git upload test repositories.\n", encoding="ascii")
    project = fixture / "project \u6e2c\u8a66 ! space"
    remote = fixture / "remote.git"
    project.mkdir()
    checks = 0

    def git(*args, cwd=project):
        r = subprocess.run([git_exe, *args], cwd=cwd, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=30)
        assert r.returncode == 0, r.stdout.decode("utf-8", errors="replace")
        return r.stdout.decode("utf-8", errors="replace").strip()

    def run():
        command = '"' + env["COMSPEC"] + '" /d /s /c ""' + str(project / "github_push.bat") + '" --no-pause"'
        r = subprocess.run(command, cwd=fixture, env=env, stdin=subprocess.DEVNULL,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
        return r.returncode, r.stdout.decode("utf-8", errors="replace")

    def passed(name):
        nonlocal checks
        checks += 1
        print("PASS " + name, flush=True)

    def local_head():
        return git("rev-parse", "HEAD")

    def remote_head():
        return git("--git-dir=" + str(remote), "rev-parse", "refs/heads/main")

    try:
        git("init", "--bare", str(remote), cwd=fixture)
        git("init", "-b", "main")
        git("config", "core.autocrlf", "false")
        hooks = fixture / "empty-hooks"
        hooks.mkdir()
        git("config", "core.hooksPath", str(hooks))
        git("remote", "add", "origin", remote.as_posix())
        script = (ROOT / "github_push.bat").read_bytes()
        assert script.count(DESTINATION.encode("ascii")) == 1
        (project / "github_push.bat").write_bytes(script.replace(DESTINATION.encode("ascii"), remote.as_posix().encode("ascii")))
        shutil.copyfile(ROOT / ".gitignore", project / ".gitignore")
        (project / "README.md").write_text("Initial fixture\n", encoding="ascii")
        (project / "obsolete.txt").write_text("old\n", encoding="ascii")
        git("add", "-A")
        git("commit", "-m", "Fixture initial")
        (project / "README.md").write_text("Updated fixture\n", encoding="ascii")
        (project / "obsolete.txt").unlink()
        (project / "\u65b0\u8a5e.txt").write_text("new\n", encoding="ascii")
        (project / "_Archive").mkdir()
        (project / "_Archive/local.bin").write_bytes(b"LOCAL_ONLY")
        (project / "MANUAL_CLEANUP.md").write_text("Local record\n", encoding="ascii")
        code, output = run()
        assert code == 0 and "SUCCESS:" in output, output
        assert local_head() == remote_head()
        tree = git("--git-dir=" + str(remote), "-c", "core.quotePath=false", "ls-tree", "-r", "--name-only", "main")
        assert "\u65b0\u8a5e.txt" in tree and "obsolete.txt" not in tree
        assert "_Archive" not in tree and "MANUAL_CLEANUP.md" not in tree
        assert git("rev-parse", "--abbrev-ref", "@{upstream}") == "origin/main"
        passed("One batch commits additions, modifications and deletions, excludes local data, and pushes with Unicode/space/! paths and no Python PATH")

        before = local_head()
        code, output = run()
        assert code == 0 and local_head() == before and remote_head() == before, output
        passed("An unchanged repeat pushes successfully without an empty commit")

        (project / "ready.txt").write_text("committed before upload\n", encoding="ascii")
        git("add", "ready.txt"); git("commit", "-m", "Fixture ready")
        before = local_head()
        code, output = run()
        assert code == 0 and local_head() == before and remote_head() == before, output
        passed("Existing local commits are pushed even when the working tree has no new changes")

        git("remote", "set-url", "origin", (fixture / "wrong.git").as_posix())
        (project / "pending.txt").write_text("not yet staged\n", encoding="ascii")
        before = local_head()
        code, output = run()
        assert code != 0 and local_head() == before and not git("diff", "--cached", "--name-only"), output
        git("remote", "set-url", "origin", remote.as_posix())
        passed("A different destination is rejected before staging or committing")

        git("config", "--add", "remote.origin.pushurl", remote.as_posix())
        git("config", "--add", "remote.origin.pushurl", (fixture / "second.git").as_posix())
        code, output = run()
        assert code != 0 and local_head() == before, output
        git("config", "--unset-all", "remote.origin.pushurl")
        passed("Multiple push URLs are rejected before publication")

        git("switch", "-c", "feature")
        code, output = run()
        assert code != 0 and "branch must be main" in output, output
        git("switch", "main")
        passed("An unintended branch is not uploaded")

        merge_head = project / ".git/MERGE_HEAD"
        merge_head.write_text(local_head() + "\n", encoding="ascii")
        code, output = run()
        assert code != 0 and "existing Git" in output and not git("diff", "--cached", "--name-only"), output
        merge_head.unlink()
        passed("An unfinished Git operation is preserved for explicit resolution")

        failing_hooks = fixture / "failing-hooks"
        failing_hooks.mkdir()
        (failing_hooks / "pre-commit").write_text("#!/bin/sh\nexit 1\n", encoding="ascii")
        git("config", "core.hooksPath", str(failing_hooks))
        code, output = run()
        assert code != 0 and local_head() == before and remote_head() == before, output
        assert "pending.txt" in git("diff", "--cached", "--name-only")
        git("config", "core.hooksPath", str(hooks))
        passed("Commit failure stops the push and preserves the staged change")

        other = fixture / "another"
        git("clone", "--branch", "main", str(remote), str(other), cwd=fixture)
        (other / "server.txt").write_text("remote advancement\n", encoding="ascii")
        git("add", "server.txt", cwd=other); git("commit", "-m", "Fixture remote", cwd=other)
        git("push", "origin", "main", cwd=other)
        advanced = remote_head()
        code, output = run()
        assert code != 0 and "Push failed" in output and local_head() != before, output
        assert remote_head() == advanced and git("show", "HEAD:pending.txt") == "not yet staged"
        passed("A rejected push retains the local commit and does not overwrite remote history")

        git("fetch", "origin"); git("merge", "--no-edit", "origin/main")
        merged = local_head()
        code, output = run()
        assert code == 0 and remote_head() == merged and local_head() == merged, output
        passed("After resolving remote divergence, rerunning the same batch uploads the existing commit")

        (project / "_Archive/forced.bin").write_bytes(b"FORCED_LOCAL_DATA")
        git("add", "-f", "_Archive/forced.bin")
        before = local_head()
        code, output = run()
        assert code != 0 and "Ignored files" in output and local_head() == before and remote_head() == before, output
        passed("Force-added ignored records are stopped before commit/push")
    finally:
        assert fixture.parent == TEST.resolve() and marker.is_file()
        for directory, dirs, files in os.walk(fixture, followlinks=False):
            for name in dirs + files:
                assert not (Path(directory, name).lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT)

        def remove_readonly(function, path, _error):
            target = Path(path).resolve()
            assert target.is_relative_to(fixture)
            target.chmod(stat.S_IWRITE)
            function(path)

        shutil.rmtree(fixture, onerror=remove_readonly)
    print(f"PASS {checks} upload checks; owned local repositories removed; GitHub was not contacted", flush=True)


if __name__ == "__main__":
    main()
