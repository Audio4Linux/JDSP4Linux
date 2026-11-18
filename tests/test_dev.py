import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("dev", Path(__file__).resolve().parents[1] / "tools/dev.py")
dev = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dev)


class DeveloperTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        (self.root / "src").mkdir()
        self.binary = self.root / "src/jamesdsp"
        self.binary.write_text("#!/bin/sh\nif [ \"$1\" = --is-connected ]; then exit 1; fi\nexec sleep 60\n")
        self.binary.chmod(0o755)
        self.developer = dev.Developer(self.root, self.root, "qmake", 1, ["--tray"])
        self.build = patch.object(self.developer, "run_build", return_value=True)
        self.build.start()

    def tearDown(self):
        self.developer.close()
        self.build.stop()
        self.directory.cleanup()

    def test_build_failure_keeps_running_process_and_binary(self):
        self.assertTrue(self.developer.rebuild())
        first = self.developer.app
        before = self.developer.runtime.read_bytes()
        with patch.object(self.developer, "run_build", return_value=False):
            self.assertFalse(self.developer.rebuild())
        self.assertIs(self.developer.app, first)
        self.assertIsNone(first.poll())
        self.assertEqual(before, self.developer.runtime.read_bytes())

    def test_copy_failure_keeps_running_process(self):
        self.developer.rebuild()
        first = self.developer.app
        with patch.object(dev.shutil, "copy2", side_effect=OSError("copy failed")):
            with self.assertRaisesRegex(OSError, "copy failed"):
                self.developer.rebuild()
        self.assertIsNone(first.poll())

    def test_success_replaces_only_owned_process(self):
        self.developer.rebuild()
        first = self.developer.app
        self.binary.write_text(self.binary.read_text() + "# new source build\n")
        self.developer.rebuild()
        self.assertIsNotNone(first.poll())
        self.assertNotEqual(first.pid, self.developer.app.pid)
        self.assertIsNone(self.developer.app.poll())
        self.assertEqual(self.binary.read_bytes(), self.developer.runtime.read_bytes())
        self.assertEqual(self.developer.app.args[1:], ["--watch-liveprog", "--tray"])
        self.developer.close()
        self.assertIsNotNone(self.developer.app.poll())

    def test_publish_failure_keeps_running_process(self):
        self.developer.rebuild()
        first = self.developer.app
        with patch.object(dev.os, "replace", side_effect=OSError("rename failed")):
            with self.assertRaisesRegex(OSError, "rename failed"):
                self.developer.rebuild()
        self.assertIsNone(first.poll())

    def test_refuses_existing_service(self):
        self.binary.write_text("#!/bin/sh\nexit 0\n")
        with self.assertRaisesRegex(RuntimeError, "Another JamesDSP process"):
            self.developer.rebuild()
        self.assertIsNone(self.developer.app)

    def test_initial_failure_does_not_start_audio(self):
        with patch.object(self.developer, "run_build", return_value=False):
            self.assertFalse(self.developer.rebuild())
        self.assertIsNone(self.developer.app)
        self.assertFalse(self.developer.runtime.exists())


if __name__ == "__main__":
    unittest.main()
