#!/usr/bin/env python3
"""Build on source file events and restart only this script's JamesDSP child."""

import argparse
import fcntl
import os
from pathlib import Path
import queue
import re
import shutil
import signal
import subprocess
import sys
import threading


class Developer:
    def __init__(self, source, build, qmake, jobs, app_args, on_exit=None):
        self.source = source
        self.build = build
        self.qmake = qmake
        self.jobs = jobs
        self.app_args = app_args
        self.on_exit = on_exit
        self.app = None
        self.worker = None
        self.app_log = None
        self.runtime = build / "run" / "jamesdsp"

    @staticmethod
    def stop(process):
        if process is not None and process.poll() is None:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                process.wait()
                return
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait()

    def run_build(self, command, log):
        self.worker = subprocess.Popen(command, cwd=self.build, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
        return self.worker.wait() == 0

    def rebuild(self):
        print("Building; current audio process stays active.", flush=True)
        with (self.build / "build.log").open("w") as log:
            success = self.run_build([self.qmake, str(self.source / "JDSP4Linux.pro")], log)
            if success:
                success = self.run_build(["make", f"-j{self.jobs}"], log)
        if not success:
            print(f"Build failed. Current process unchanged. Read {self.build / 'build.log'}", flush=True)
            return False

        binary = self.build / "src" / "jamesdsp"
        # Do not take over a process started by the desktop or another developer.
        if self.app is None:
            connected = subprocess.run([str(binary), "--is-connected"],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if connected.returncode == 0:
                raise RuntimeError("Another JamesDSP process is active. Stop that process before starting this loop.")

        self.runtime.parent.mkdir(exist_ok=True)
        candidate = self.runtime.with_suffix(".next")
        shutil.copy2(binary, candidate)
        if self.app_log is None:
            self.app_log = (self.build / "application.log").open("a", buffering=1)
        # Publish before stopping. A failed build, copy, or rename cannot stop audio.
        os.replace(candidate, self.runtime)
        self.stop(self.app)
        self.app = subprocess.Popen([str(self.runtime), "--watch-liveprog", *self.app_args],
                                    stdout=self.app_log, stderr=subprocess.STDOUT,
                                    start_new_session=True)
        if self.on_exit:
            def wait_for_exit(process):
                process.wait()
                self.on_exit(process)
            threading.Thread(target=wait_for_exit, args=(self.app,), daemon=True).start()
        print(f"Started source build, PID {self.app.pid}. Log: {self.build / 'application.log'}", flush=True)
        return True

    def close(self):
        self.stop(self.worker)
        self.stop(self.app)
        if self.app_log:
            self.app_log.close()


def main():
    source = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=source / "build")
    parser.add_argument("--qmake", default=shutil.which("qmake6") or "qmake")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("app_args", nargs=argparse.REMAINDER, help="JamesDSP options after --")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")
    for command in (args.qmake, "make", "inotifywait"):
        if shutil.which(command) is None:
            parser.error(f"Required command missing: {command}. Install the build dependencies and inotify-tools.")

    build = args.build_dir.resolve()
    if build == source or source.is_relative_to(build):
        parser.error("The build directory must not be the source directory or its parent.")
    build.mkdir(parents=True, exist_ok=True)
    lock = (build / ".dev.lock").open("w")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        parser.error("A development loop already uses this build directory.")

    app_args = args.app_args[1:] if args.app_args[:1] == ["--"] else args.app_args
    events = queue.Queue()
    developer = Developer(source, build, args.qmake, args.jobs, app_args, events.put)
    ready = threading.Event()
    exclude = r"(^|/)(\.git|build)(/|$)|^" + re.escape(str(build)) + r"(/|$)"
    watcher = subprocess.Popen(
        ["inotifywait", "--monitor", "--recursive", "--exclude", exclude,
         "--event", "close_write,moved_to,moved_from,create,delete", "--format", "%w%f",
         str(source)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True,
        env=dict(os.environ, LC_ALL="C"))

    def read_errors():
        for line in watcher.stderr:
            if "Watches established." in line:
                ready.set()
            else:
                print(line.rstrip(), file=sys.stderr, flush=True)
        events.put(None)

    def read_events():
        suffixes = {".c", ".cpp", ".cc", ".h", ".hpp", ".pro", ".pri", ".ui", ".qrc",
                    ".png", ".svg", ".qm", ".conf"}
        for line in watcher.stdout:
            path = Path(line.rstrip("\n"))
            # EEL scripts use --watch-liveprog and do not need a native restart.
            if path.suffix in suffixes:
                events.put(path)
        events.put(None)

    def interrupted(_signum, _frame):
        raise KeyboardInterrupt

    def next_change(timeout=None):
        while True:
            event = events.get(timeout=timeout)
            if event is None:
                raise RuntimeError("Source watcher stopped. Repair the watch failure before restarting the loop.")
            if isinstance(event, subprocess.Popen):
                if event is developer.app:
                    raise RuntimeError(f"JamesDSP exited with code {event.returncode}. Read {build / 'application.log'}")
                continue  # Exit of the previous child after a successful rebuild.
            return event

    signal.signal(signal.SIGTERM, interrupted)
    threading.Thread(target=read_errors, daemon=True).start()
    threading.Thread(target=read_events, daemon=True).start()
    try:
        if not ready.wait(timeout=10):
            raise RuntimeError("Source watch failed to start. Read the inotify error; do not run without source watches.")
        developer.rebuild()
        print("Watching native source. EEL edits reload inside JamesDSP. Ctrl-C stops this loop and its child.", flush=True)
        while True:
            next_change()
            # Wait for the last event in a save burst. Events during a build stay queued.
            while True:
                try:
                    next_change(timeout=0.3)
                except queue.Empty:
                    break
            developer.rebuild()
    except KeyboardInterrupt:
        return 0
    finally:
        Developer.stop(watcher)
        developer.close()
        lock.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError) as error:
        print(f"Development loop failed: {error}", file=sys.stderr)
        sys.exit(1)
