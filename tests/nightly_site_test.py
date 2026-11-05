#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Beta, nightly and synth-only site generation preserves releases/editor and rejects mismatched builds."""
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "web"))
from make_site import main
from publish_nightly import publish
from publish_synth import publish as publish_synth
from publish_beta import publish as publish_beta


def package(path, product, variant):
    raw = bytearray(1024)
    for i in range(20):
        raw[i * 0x30 + 0x2F] = (ord(product[i]) + i + 1) & 255 if i < len(product) else 0x7D
    raw[960:976] = b"FELUCCA-LOADER-1" + bytes([variant])
    path.write_bytes(raw)
    return path


class NightlySite(unittest.TestCase):
    def test_publish(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dual = package(root / "release.fwsc", "FM-1_811", 1)
            single = package(root / "single.fwsc", "FM-1_811", 2)
            normal = package(root / "next.fwsc", "FM-1_800", 3)
            slow = package(root / "slow.fwsc", "FM-1_800", 4)   # (the single-core nightly)
            site = root / "site"
            main(dual, single, "1.1.0", site)
            home = (site / "index.html").read_text()   # the project page names the release and its packages
            self.assertIn("Install 1.1.0", home)
            self.assertIn("firmware/tessera-1.1.0-1core.fwsc", home)
            self.assertNotRegex(home, r"@[A-Z0-9]+@")
            src = Path(__file__).resolve().parent.parent / "demos"   # (the editor's Demo projects, and the CHK projects)
            for sub in ("", "checks"):
                self.assertEqual(sorted(f.name for f in (site / "webapp/editor/demos" / sub).glob("*.tsp")),
                                 sorted(f.name for f in (src / sub).glob("*.tsp")))
            editor = site / "webapp/editor/index.html"
            editor.write_text("existing published editor")
            # Exercise migration from the existing inline-only release metadata.
            (site / "firmware/release.json").unlink()
            publish(site, str(normal), str(slow), "next-abcdef0", "abcdef0" * 5 + "abcde", "2026-10-11T02:00:00Z")
            self.assertEqual(editor.read_text(), "existing published editor")
            self.assertEqual((site / "firmware/tessera-1.1.0.fwsc").read_bytes(), dual.read_bytes())
            self.assertEqual((site / "firmware/tessera-1.1.0-1core.fwsc").read_bytes(), single.read_bytes())
            manifest = json.loads((site / "firmware/nightly.json").read_text())
            self.assertEqual((manifest["product"], manifest["branch"]), ("FM-1_800", "next"))
            # Regenerating a release keeps both published nightly variants.
            main(dual, single, "1.1.0", site, installer_only=True)
            self.assertIn('"single": "../../firmware/melodee-nightly-1core.fwsc"', (site / "webapp/installer/index.html").read_text())
            self.assertEqual((site / "firmware/melodee-nightly-1core.fwsc").read_bytes(), slow.read_bytes())
            before = hashlib.sha256((site / "webapp/installer/index.html").read_bytes()).digest()
            wrong = package(root / "wrong.fwsc", "FM-1_810", 5)
            official = package(root / "official.fwsc", "FM-1_015", 9)   # not a Tessera identity
            for other in (wrong, normal, official):
                with self.assertRaises(SystemExit):
                    publish(site, str(normal), str(other), "next-abcdef0", "abcdef0" * 5 + "abcde", "2026-10-11T02:00:00Z")
                self.assertEqual(hashlib.sha256((site / "webapp/installer/index.html").read_bytes()).digest(), before)

    def test_synth(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dual = package(root / "release.fwsc", "FM-1_91010", 1)
            single = package(root / "single.fwsc", "FM-1_91010", 2)
            normal = package(root / "next.fwsc", "FM-1_900", 3)
            slow = package(root / "slow.fwsc", "FM-1_900", 4)
            synth = package(root / "synth.fwsc", "FM-1_900", 6)
            synth1 = package(root / "synth-1core.fwsc", "FM-1_900", 7)
            web = root / "synth-web"
            web.mkdir()
            for f in ("editor.html", "fukiai.ttf", "FUKIAI-LICENSE.txt", "fm1backup.js"):
                (web / f).write_text("synth " + f)
            site = root / "site"
            main(dual, single, "1.1.0", site)
            editor = site / "webapp/editor/index.html"
            editor.write_text("existing published editor")
            installer = site / "webapp/installer/index.html"
            self.assertIn('"synth": null', installer.read_text())
            commit = "1234567" * 5 + "12345"
            publish_synth(site, str(synth), str(synth1), "synth-1234567", commit, "2026-10-12T08:00:00Z", str(web))
            self.assertEqual(editor.read_text(), "existing published editor")
            self.assertEqual((site / "webapp/synth-editor/index.html").read_text(), "synth editor.html")
            self.assertEqual((site / "webapp/synth-editor/fm1backup.js").read_text(), "synth fm1backup.js")
            manifest = json.loads((site / "firmware/synth.json").read_text())
            self.assertEqual((manifest["product"], manifest["branch"], manifest["commit"]), ("FM-1_900", "synth-only", commit))
            self.assertIn('"single": "../../firmware/melodee-synth-1core.fwsc"', installer.read_text())
            # A nightly run and a regenerated release both keep the synth-only firmware.
            publish(site, str(normal), str(slow), "next-abcdef0", "abcdef0" * 5 + "abcde", "2026-10-11T02:00:00Z")
            main(dual, single, "1.1.0", site, installer_only=True)
            self.assertEqual((site / "firmware/melodee-synth.fwsc").read_bytes(), synth.read_bytes())
            self.assertEqual((site / "firmware/melodee-synth-1core.fwsc").read_bytes(), synth1.read_bytes())
            self.assertEqual((site / "firmware/melodee-nightly-1core.fwsc").read_bytes(), slow.read_bytes())
            html = installer.read_text()
            self.assertIn('"version": "synth-1234567"', html)
            self.assertIn('"editor": "../synth-editor/"', html)
            self.assertIn('"single": "../../firmware/melodee-nightly-1core.fwsc"', html)
            before = hashlib.sha256(installer.read_bytes()).digest()
            wrong = package(root / "wrong.fwsc", "FM-1_9010", 8)
            for other in (wrong, synth):
                with self.assertRaises(SystemExit):
                    publish_synth(site, str(synth), str(other), "synth-1234567", commit, "2026-10-12T08:00:00Z", str(web))
                self.assertEqual(hashlib.sha256(installer.read_bytes()).digest(), before)

    def test_beta(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dual = package(root / "release.fwsc", "FM-1_83773022", 1)
            single = package(root / "single.fwsc", "FM-1_83773022", 2)
            beta = package(root / "beta.fwsc", "FM-1_833", 3)
            beta1 = package(root / "beta-1core.fwsc", "FM-1_833", 4)
            site = root / "site"
            main(dual, single, "3.2.2", site)
            editor, installer = site / "webapp/editor/index.html", site / "webapp/installer/index.html"
            editor.write_text("existing published editor")
            self.assertIn('"beta": null', installer.read_text())
            commit = "89abcde" * 5 + "89abc"
            publish_beta(site, str(beta), str(beta1), "3.3-beta1", commit, "2026-11-04T19:00:00Z")
            self.assertEqual(editor.read_text(), "existing published editor")
            self.assertEqual((site / "firmware/tessera-3.2.2.fwsc").read_bytes(), dual.read_bytes())
            self.assertEqual((site / "firmware/tessera-3.3-beta1-1core.fwsc").read_bytes(), beta1.read_bytes())
            manifest = json.loads((site / "firmware/beta.json").read_text())
            self.assertEqual((manifest["version"], manifest["product"], manifest["branch"], manifest["commit"]),
                             ("3.3-beta1", "FM-1_833", "develop", commit))
            self.assertIn('"dual": "../../firmware/tessera-3.3-beta1.fwsc"', installer.read_text())
            # A second beta replaces the first; a 3.2.x patch release keeps it.
            publish_beta(site, str(beta), str(beta1), "3.3-beta2", commit, "2026-11-05T19:00:00Z")
            self.assertFalse((site / "firmware/tessera-3.3-beta1.fwsc").exists())
            main(dual, single, "3.2.3", site, installer_only=True)
            self.assertIn('"version": "3.3-beta2"', installer.read_text())
            self.assertTrue((site / "firmware/tessera-3.3-beta2.fwsc").exists())
            before = hashlib.sha256(installer.read_bytes()).digest()
            wrong = package(root / "wrong.fwsc", "FM-1_834", 5)
            for other, version in ((wrong, "3.3-beta3"), (beta, "3.3-beta3"), (beta1, "3.3")):
                with self.assertRaises(SystemExit):   # (mixed identities, one build twice, no suffix)
                    publish_beta(site, str(beta), str(other), version, commit, "2026-11-06T19:00:00Z")
                self.assertEqual(hashlib.sha256(installer.read_bytes()).digest(), before)
            # The release it leads to ends the beta: its packages and manifest go.
            main(dual, single, "3.3", site, installer_only=True)
            self.assertIn('"beta": null', installer.read_text())
            self.assertFalse((site / "firmware/beta.json").exists())
            self.assertFalse(any(site.glob("firmware/tessera-3.3-beta*.fwsc")))
            # --off takes a beta off before then.
            publish_beta(site, str(beta), str(beta1), "3.4-beta1", commit, "2026-11-07T19:00:00Z")
            publish_beta(site, off=True)
            self.assertIn('"beta": null', installer.read_text())
            self.assertFalse(any(site.glob("firmware/tessera-3.4-beta*.fwsc")))


if __name__ == "__main__":
    unittest.main()
