#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The manual as a PDF to download, print or read offline: MANUAL.md on A4.

  tools/gen_manual_pdf.py [--pdf BROWSER] [--html-only]

MANUAL.md stays the source: this turns it into a page for paper (build/manual-pdf/manual.html: black ink on white,
the headings in Tessera Mono as on the cheatsheet, a cover, the contents, every "More detail" fold opened, page
numbers at the foot) and prints it headless with a Chromium-based browser (Chrome, Edge, Chromium) to
docs/manual/tessera-manual.pdf, with the chapters as the PDF's bookmarks. Links inside the manual stay links in the
PDF; links to the repository's other files go to them on GitHub.

Run it after a change to MANUAL.md or its pictures, and commit the PDF. web/make_site.py copies it to the site's
manual/tessera-manual.pdf, which the web manual's PDF link points to.

BROWSER: the browser's program. Left out, the first of chromium, chromium-browser, google-chrome, msedge on the
PATH (and the usual Windows / macOS places). Needs markdown-it-py and fonttools: pip install markdown-it-py fonttools
"""
import argparse
import base64
import html
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "web"))
from gen_mono_webfont import build as mono_woff         # noqa: E402
import make_manual                                      # noqa: E402

SRC = ROOT / "MANUAL.md"
WORK = ROOT / "build" / "manual-pdf"
PDF = ROOT / "docs" / "manual" / "tessera-manual.pdf"
REPO = make_manual.REPO
SITE = "https://rastapopolix.github.io/tessera/"

CSS = """
@page { size: A4; margin: 16mm 16mm 18mm 16mm;
  @bottom-left { content: "Tessera manual"; font: 7.5pt/1 "Segoe UI", "Helvetica Neue", Arial, sans-serif; color: #777; }
  @bottom-right { content: counter(page); font: 8pt/1 "Segoe UI", "Helvetica Neue", Arial, sans-serif; color: #444; } }
@page :first { margin: 0; @bottom-left { content: none; } @bottom-right { content: none; } }
:root { --ink: #000; --line: #c8c8c8; --tint: #f2f2f2; --pixel: "Tessera Mono", ui-monospace, monospace; }
* { box-sizing: border-box; }
html { -webkit-print-color-adjust: exact; print-color-adjust: exact; }
body { margin: 0; color: var(--ink); background: #fff; font: 9.5pt/1.5 "Segoe UI", "Helvetica Neue", Arial, sans-serif; }
a { color: inherit; text-decoration: underline; text-decoration-color: #999; text-underline-offset: 2px; }
.cover { height: 297mm; padding: 34mm 22mm 20mm; display: flex; flex-direction: column; break-after: page; }
.cover .banner { width: 100%; image-rendering: pixelated; }
.cover .title { margin: 22mm 0 4mm; font: 400 40pt/1 var(--pixel); text-transform: uppercase; }
.cover .sub { margin: 0; font-size: 13pt; color: #333; }
.cover .foot { margin-top: auto; font-size: 8.5pt; color: #555; }
.cover .foot p { margin: 1mm 0; }
nav.toc { break-after: page; }
nav.toc h2 { border: 0; margin-top: 0; }
nav.toc ol { list-style: none; margin: 0; padding: 0; }
nav.toc li.group { margin: 4mm 0 1mm; padding: .6mm 1.6mm .4mm; color: #fff; background: var(--ink);
  font: 400 8.5pt/1.1 var(--pixel); text-transform: uppercase; break-after: avoid; }
nav.toc li.ch { margin: .6mm 0; }
nav.toc li.ch > a { text-decoration: none; font-weight: 600; }
nav.toc li.ch ol { padding-left: 5mm; font-size: 8.5pt; color: #333; columns: 2; column-gap: 6mm; }
nav.toc li.ch ol a { text-decoration: none; }
main h1, main h2, main h3, main h4 { font-family: var(--pixel); font-weight: 400; line-height: 1.2; break-after: avoid; }
main h1 { display: none; }
main h2 { font-size: 17pt; margin: 0 0 4mm; padding: 1.4mm 2.4mm 1mm; color: #fff; background: var(--ink); break-before: page; }
main h3 { font-size: 12pt; margin: 7mm 0 2mm; padding-bottom: .8mm; border-bottom: .4mm solid var(--ink); }
main h4 { font-size: 10pt; margin: 5mm 0 1.5mm; }
main p { margin: 0 0 2.4mm; orphans: 3; widows: 3; }
main ul, main ol { margin: 0 0 2.4mm; padding-left: 6mm; }
main li { margin: 0 0 1.2mm; }
main li > p { margin: 1mm 0; }
main img { max-width: 62%; height: auto; image-rendering: pixelated; break-inside: avoid; }
main p[align="center"] { text-align: center; break-inside: avoid; }
main p[align="center"] img { display: inline-block; margin: 1mm; max-width: 48%; }
main sub { color: #555; font-size: 7.5pt; }
main table { width: 100%; border-collapse: collapse; margin: 2mm 0 3.5mm; font-size: 8.5pt; line-height: 1.35; }
main thead { display: table-header-group; }
main tr { break-inside: avoid; }
main th, main td { border: .25mm solid var(--line); padding: 1mm 1.6mm; text-align: left; vertical-align: top; }
main th { background: var(--ink); color: #fff; font-weight: 600; border-color: var(--ink); }
main tr:nth-child(even) td { background: var(--tint); }
main code { font: 8.5pt/1.3 ui-monospace, Consolas, "Liberation Mono", monospace; background: var(--tint); padding: 0 1mm; }
main pre { background: var(--tint); padding: 2mm 3mm; white-space: pre-wrap; break-inside: avoid; }
main pre code { padding: 0; }
main blockquote { margin: 2.5mm 0; padding: 2mm 3.5mm; border-left: 1.2mm solid var(--ink); background: var(--tint); break-inside: avoid; }
main blockquote p { margin: 0; }
main details { margin: 2.5mm 0; padding: 1.5mm 3.5mm; border: .3mm solid var(--line); }
main summary { font-weight: 600; list-style: none; margin-bottom: 1mm; }
main summary::-webkit-details-marker { display: none; }
main hr { border: 0; border-top: .3mm solid var(--line); margin: 5mm 0; }
"""


def link(url):
    """a link or picture of MANUAL.md, as the PDF reaches it: the manual's own anchors in the PDF, the pictures from
    the checkout, the repository's other files on GitHub"""
    if re.match(r"^(https?:|mailto:|#|data:)", url):
        return url
    path, _, frag = url.partition("#")
    path = path.lstrip("./")
    if path == "MANUAL.md":
        return f"#{frag}" if frag else "#top"
    if path == "GETTING_STARTED.md":
        return SITE + "start/" + (f"#{frag}" if frag else "")
    if path.startswith("assets/"):
        return (ROOT / path).resolve().as_uri()
    return REPO + path + (f"#{frag}" if frag else "")


def convert(md_text):
    """the manual as HTML: GitHub's heading ids (make_manual's), the links the PDF's, the folds open"""
    body, heads = make_manual.convert(md_text, "manual")
    # make_manual already turned the links into the site's: redo them for paper from the markdown's own
    raw = make_manual.MD.render(md_text)
    srcs = re.findall(r'(?:href|src)="([^"]*)"', raw)
    it = iter(srcs)
    body = re.sub(r'(href|src)="[^"]*"', lambda m: f'{m.group(1)}="{html.escape(link(html.unescape(next(it))))}"', body)
    body = re.sub(r"<details(?![^>]*\bopen\b)", "<details open", body)
    return body, heads


def contents(heads):
    """the contents page: the web manual's groups (make_manual.GROUPS), each chapter with its sections"""
    chapters, subs, titles, cur = [], {}, {}, None
    for level, hid, text in heads:
        if level in (2, 3):
            titles[hid] = text
        if level == 2:
            cur = hid
            chapters.append(hid)
            subs[hid] = []
        elif level == 3 and cur:
            subs[cur].append(hid)
    groups = make_manual.GROUPS["manual"]
    moved = {hid for _, ids in groups for hid in ids if hid not in subs}
    rest = [hid for hid in chapters if not any(hid in ids for _, ids in groups)]
    if rest:
        groups = groups + [("More", rest)]
    out = ['<nav class="toc"><h2 id="contents">Contents</h2><ol>']
    for label, ids in groups:
        out.append(f'<li class="group">{html.escape(label)}</li>')
        for hid in ids:
            if hid not in titles:
                continue
            kids = "".join(f'<li><a href="#{k}">{html.escape(titles[k])}</a></li>' for k in subs.get(hid, []) if k not in moved)
            out.append(f'<li class="ch"><a href="#{hid}">{html.escape(titles[hid])}</a>' + (f"<ol>{kids}</ol>" if kids else "") + "</li>")
    out.append("</ol></nav>")
    return "".join(out)


def version():
    """the release the manual describes: the newest docs/RELEASE_X.Y[.Z].md"""
    vs = [tuple(int(n) for n in m.group(1).split(".")) for f in (ROOT / "docs").glob("RELEASE_*.md")
          if (m := re.fullmatch(r"RELEASE_([\d.]+)\.md", f.name))]
    return ".".join(map(str, max(vs))) if vs else ""


def page():
    md_text = SRC.read_text(encoding="utf-8")
    # the banner goes on the cover; the markdown's own copy of it is left out
    md_text = re.sub(r'^<p align="center"><img src="assets/brand/banner.png"[^\n]*</p>\n', "", md_text, count=1)
    md_text = make_manual.videos(md_text, "text")      # the guided clips as a line of links, not thumbnails
    body, heads = convert(md_text)
    v = version()
    face = ('@font-face { font-family: "Tessera Mono"; src: url("data:font/woff;base64,'
            + base64.b64encode(mono_woff()).decode() + '") format("woff"); }')
    banner = (ROOT / "assets" / "brand" / "banner.png").resolve().as_uri()
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Tessera manual</title>
<!-- made by tools/gen_manual_pdf.py from MANUAL.md: edit that, not this -->
<style>{face}{CSS}</style>
</head>
<body id="top">
<section class="cover">
<img class="banner" src="{banner}" alt="Tessera: generative firmware for the M-Vave FM-1">
<p class="title">Tessera manual</p>
<p class="sub">Generative firmware for the M-Vave FM-1{f", version {v}" if v else ""}</p>
<div class="foot">
<p>The latest manual, always: <a href="{SITE}manual/">{SITE}manual/</a></p>
<p>Recipes: <a href="{REPO}RECIPES.md">RECIPES.md</a> · Every button on one sheet: <a href="{REPO}docs/cheatsheet/tessera-cheatsheet.pdf">the cheatsheet</a></p>
<p>Tessera is free software under the GPL-3.0. See LICENSING.md for the parts under other licences.</p>
</div>
</section>
{contents(heads)}
<main>
{body}
</main>
</body>
</html>
"""


def find_browser():
    for name in ("chromium", "chromium-browser", "google-chrome", "google-chrome-stable", "msedge", "chrome"):
        p = shutil.which(name)
        if p:
            return p
    for p in (r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
              r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
              r"C:\Program Files\Google\Chrome\Application\chrome.exe",
              "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
              "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge",
              "/opt/pw-browsers/chromium/chrome-linux/chrome"):
        if os.path.exists(p):
            return p
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--pdf", metavar="BROWSER", help="the Chromium-based browser to print with")
    ap.add_argument("--html-only", action="store_true", help="write the page, print no PDF")
    a = ap.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    page_path = WORK / "manual.html"
    page_path.write_text(page(), encoding="utf-8")
    print(page_path)
    if a.html_only:
        return 0
    browser = a.pdf or find_browser()
    if not browser:
        sys.exit("gen_manual_pdf: no Chromium-based browser found: give one with --pdf")
    PDF.parent.mkdir(parents=True, exist_ok=True)
    cmd = [browser, "--headless", "--disable-gpu", "--no-pdf-header-footer", "--generate-pdf-document-outline",
           "--allow-file-access-from-files", f"--print-to-pdf={PDF}", page_path.resolve().as_uri()]
    if os.name != "nt" and hasattr(os, "geteuid") and os.geteuid() == 0:
        cmd.insert(1, "--no-sandbox")               # (a browser run as root, as in a container, needs it)
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(f"{PDF} ({PDF.stat().st_size // 1024} KB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
