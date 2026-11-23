#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the site's manual pages (GitHub Pages), from the repository's markdown:
  manual/index.html   MANUAL.md
  start/index.html    GETTING_STARTED.md
  plugin/index.html   PLUGIN.md: what differs in the plugin, with its downloads at the top (plugin_release.json)

Each page: the project page's look (Tessera Mono and the colours from web/home/index.html, the light / dark choice it
stores, tessera-theme), a sidebar of the chapters (the ## headings, the open one's ### under it; the manual's grouped by button) and a search over the
page's text, with no network. Headings keep GitHub's anchors (lower case, punctuation dropped, spaces as hyphens, -1
-2 on repeats), so a link to MANUAL.md#a-section on GitHub works here as manual/#a-section.

Links: MANUAL.md, GETTING_STARTED.md and PLUGIN.md to these pages; the manual's PDF (docs/manual/tessera-manual.pdf, made by
tools/gen_manual_pdf.py) to its copy at manual/tessera-manual.pdf (make_site.py puts it there); every other file of
the repository (RECIPES.md, docs/...) to it on GitHub; the pictures (assets/gallery, assets/brand, assets/guides, assets/plugin) to the site's media/ (web/make_site.py copies them there).

  web/make_manual.py OUT_DIR            (web/make_site.py calls it; needs markdown-it-py: pip install markdown-it-py)
"""
import html
import json
import re
import sys
from pathlib import Path

from markdown_it import MarkdownIt

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
REPO = "https://github.com/Rastapopolix/tessera/blob/HEAD/"
PAGES = {"MANUAL.md": "manual", "GETTING_STARTED.md": "start", "PLUGIN.md": "plugin"}
# CommonMark, as GitHub reads it (a list item's lines indented by two, raw HTML kept), with its tables
MD = MarkdownIt("commonmark", {"html": True}).enable("table")
PDF_SRC = "docs/manual/tessera-manual.pdf"          # the manual as a PDF (tools/gen_manual_pdf.py)
PDF_NAME = "tessera-manual.pdf"                     # its name on the site, next to the manual's page
TITLES = {"manual": "Tessera manual", "start": "Getting started with Tessera", "plugin": "Cosmati: Tessera as a plugin"}


def slug(text, seen):
    """GitHub's anchor for a heading's text: lower case, punctuation dropped, spaces as hyphens, -1 -2 on repeats"""
    s = re.sub(r"[^\w\- ]", "", text.strip().lower()).replace(" ", "-")
    n = seen.get(s, 0)
    seen[s] = n + 1
    return s if not n else f"{s}-{n}"


def link(url, here):
    """a link or picture of the markdown, as the site page `here` (manual, start) reaches it"""
    if re.match(r"^(https?:|mailto:|#|data:)", url):
        return url
    path, _, frag = url.partition("#")
    path = path.lstrip("./")
    if path in PAGES:
        return f"../{PAGES[path]}/" + (f"#{frag}" if frag else "")
    if path == PDF_SRC:
        return "../manual/" + PDF_NAME
    for src, dst in (("assets/gallery/", "media/gallery/"), ("assets/brand/", "media/brand/"), ("assets/guides/", "media/guides/"),
                     ("assets/plugin/", "media/plugin/")):
        if path.startswith(src):
            return "../" + dst + path[len(src):]
    return REPO + path + (f"#{frag}" if frag else "")


GUIDES = ROOT / "tools/guide_videos.json"           # the guided clips (tools/guide_links.py writes their lines)
GUIDE_MARK = "<!-- guide videos -->"


def videos(md_text, mode):
    """the guided clips' lines of MANUAL.md (thumbnails linking to YouTube, as GitHub shows them) for the site ("web":
    click-to-load players, nothing fetched from YouTube until one is played) or the PDF ("text": a line of links)"""
    clips = {c["video"]: c for c in json.loads(GUIDES.read_text(encoding="utf-8"))["clips"]}

    def one(m):
        cs = [clips[v] for v in re.findall(r'href="https://youtu\.be/([\w-]+)"', m.group(0))]
        if mode == "text":
            links = ", ".join(f"[{c['title']}](https://youtu.be/{c['video']})" for c in cs)
            return f"**{'Video' if len(cs) == 1 else 'Videos'}:** {links}"
        figs = "".join(
            f'<figure><a class="play" href="https://youtu.be/{c["video"]}" data-video="{c["video"]}" '
            f'aria-label="Play the video: {html.escape(c["title"])}"><img src="assets/guides/{c["clip"]}.png" alt="" '
            f'width="640" height="360" loading="lazy"></a><figcaption>{html.escape(c["title"])}</figcaption></figure>'
            for c in cs)
        return f'<div class="videos">{figs}</div>'

    return re.sub(r"^<p><a href=\"https://youtu\.be/.*" + re.escape(GUIDE_MARK) + "$", one, md_text, flags=re.M)


# The plugin's downloads, one row per system: (system, its formats, its file). The release they come from is in
# web/plugin_release.json ({"tag", "version", "files": {"windows", "macos", "linux"}}), which .github/workflows/
# plugin-site.yml writes when a plugin release is published; without it (or a file) the row shows COMING SOON.
RELEASE_FILE = ROOT / "web" / "plugin_release.json"
SYSTEMS = [
    ("Windows", "CLAP, VST3 and the standalone app", "windows"),
    ("macOS", "CLAP, VST3, Audio Unit and the standalone app", "macos"),
    ("Linux", "CLAP and VST3", "linux"),
]
REPO_URL = "https://github.com/Rastapopolix/tessera"


def plugin_release():
    """the release the downloads come from: its version and a URL per system (None: none yet)"""
    try:
        r = json.loads(RELEASE_FILE.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        r = {}
    files = r.get("files") or {}
    urls = {key: f"{REPO_URL}/releases/download/{r['tag']}/{files[key]}" if r.get("tag") and files.get(key) else None
            for _, _, key in SYSTEMS}
    return r.get("version"), urls


DOWNLOADS_NOTE = ("The plugin is still being tested. Its public beta opens once the larger window, EXPANDED, is in. "
                  'Until then, <a href="../webapp/sim/">try Tessera in your browser</a>: the same firmware on an '
                  "on-screen FM-1.")
# PLUGIN.md's lines between these marks (a note for GitHub) become the download block on the site
DL_MARKS = re.compile(r"^<!-- downloads\b.*?-->$.*?^<!-- /downloads -->$", re.M | re.S)


def downloads(md_text):
    """PLUGIN.md with its downloads note swapped for the site's download block: a Download heading (in the
    contents), a row per system, and a link or COMING SOON for each"""
    version, urls = plugin_release()
    soon = all(url is None for url in urls.values())
    rows = "".join(
        f'<li><b>{html.escape(name)}</b><span>{html.escape(formats)}</span>'
        + (f'<a class="get" href="{html.escape(urls[key])}" download>DOWNLOAD</a>' if urls[key]
           else '<span class="get soon">COMING SOON</span>')
        + "</li>" for name, formats, key in SYSTEMS)
    note = DOWNLOADS_NOTE if soon else (
        f'<b>{html.escape(version or "")}, public beta:</b> it still has some rough edges, so please '
        f'<a href="{REPO_URL}/issues">report what you find</a>. Its notes and every version are under the '
        f'<a href="{REPO_URL}/releases">releases</a> (marked <i>Pre-release</i>).')
    block = (f'## Download\n\n<div class="downloads"><ul>{rows}</ul><p>{note} Free and open source, GPL-3.0. '
             'How to install: see <a href="#install">Install</a>.</p></div>')
    out, n = DL_MARKS.subn(lambda m: block, md_text)
    if not n:
        print("make_manual: PLUGIN.md has no <!-- downloads --> marks; the plugin page has no download block", file=sys.stderr)
    return out


def convert(md_text, here):
    """the markdown as HTML, the headings' ids GitHub's, the links the site's; and the headings (level, id, text)"""
    body = MD.render(md_text)
    body = re.sub(r'(href|src)="([^"]*)"', lambda m: f'{m.group(1)}="{html.escape(link(html.unescape(m.group(2)), here))}"', body)
    seen, heads = {}, []

    def head(m):
        level, inner = int(m.group(1)), m.group(2)
        text = html.unescape(re.sub(r"<[^>]+>", "", inner))
        hid = slug(text, seen)
        heads.append((level, hid, text))
        return f'<h{level} id="{hid}">{inner}</h{level}>'

    body = re.sub(r"<h([1-4])>(.*?)</h\1>", head, body, flags=re.S)
    return body, heads


def font_and_colours():
    """the project page's font face and its :root colours (one look across the site)"""
    home = (HERE / "home" / "index.html").read_text(encoding="utf-8")
    face = re.search(r"@font-face \{[^}]*\}", home).group(0)
    dark = re.search(r":root \{(.*?)\n\}", home, re.S).group(1)
    light = re.search(r':root\[data-theme="light"\] \{[^}]*\}', home).group(0)
    return face, dark, light


# The manual's sidebar, grouped by the FM-1 button that reaches each chapter: (label, heading ids). A ### id listed
# here moves out of its chapter into this group (GLO's pages are written up under FORM). A chapter left out goes
# at the end, and an id that no longer exists is reported: keep this in step with MANUAL.md's headings.
GROUPS = {"manual": [
    ("START", ["tessera-in-plain-words", "the-panel-at-a-glance", "shortcuts"]),
    ("EDIT: the sounds", ["sounds"]),
    ("SEQ: what a track plays", ["seq-the-mode-and-its-pages", "the-generators", "the-gen-layer"]),
    ("ARP: shaping the notes", ["evolve", "form", "tweak", "tweak-2", "length", "timing", "energy"]),
    ("GLO: the long form", ["piece-a-length-and-a-shape", "chain-one-form-for-the-project",
                            "entries-tracks-out-and-back-in", "perf-four-macros", "landscape-rain-on-water"]),
    ("REC", ["print-and-keep"]),
    ("SEL: harmony", ["harmony-one-key-chord-and-qnt-chrd"]),
    ("LFO and FX: movement", ["mods-the-modulation-cords", "matrix", "organic-tide-and-swarm", "mod-shortcut"]),
    ("HOME and SAVE", ["menu-and-settings", "saving-and-older-projects"]),
    ("HELP", ["what-a-knob-does", "if-something-goes-wrong", "glossary", "credits"]),
]}


def sidebar(heads, page):
    """the chapters (##), each with its sections (###), folded but for the open one (the script opens it);
    on the manual, under a label per button (GROUPS)"""
    chapters, subs, cur = [], {}, None
    for level, hid, text in heads:
        if level == 2:
            cur = hid
            chapters.append((hid, text))
            subs[hid] = []
        elif level == 3 and cur:
            subs[cur].append((hid, text))
    titles = {hid: text for level, hid, text in heads if level in (2, 3)}
    groups = GROUPS.get(page, [("", [hid for hid, _ in chapters])])
    moved = {hid for _, ids in groups for hid in ids if hid not in subs}
    for _, ids in groups:
        for hid in ids:
            if hid not in titles:
                print(f"make_manual: GROUPS names #{hid}, which {page} no longer has", file=sys.stderr)
    rest = [hid for hid, _ in chapters if not any(hid in ids for _, ids in groups)]
    if rest:
        groups = groups + [("", rest)]

    def item(hid):
        kids = "".join(f'<li><a href="#{k}">{html.escape(t)}</a></li>' for k, t in subs.get(hid, []) if k not in moved)
        return f'<li><a href="#{hid}">{html.escape(titles[hid])}</a>' + (f"<ol>{kids}</ol>" if kids else "") + "</li>"

    out = []
    for label, ids in groups:
        if label:
            out.append(f'<li class="group">{html.escape(label)}</li>')
        out.extend(item(hid) for hid in ids if hid in titles)
    return "<ol class=\"toc\">" + "".join(out) + "</ol>"


PAGE = """<!doctype html>
<!-- SPDX-License-Identifier: GPL-3.0-only -->
<!-- Made by web/make_manual.py from {src}: edit that file, not this one. -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<link rel="icon" href="../media/brand/logo.svg" type="image/svg+xml">
<script>try {{ var t = localStorage.getItem("tessera-theme"); if (t) document.documentElement.dataset.theme = t; }} catch (e) {{}}</script>
<style>
{face}
:root {{{dark}
}}
{light}
* {{ box-sizing: border-box; }}
html {{ scroll-padding-top: 64px; }}
body {{ margin: 0; background: var(--bg); color: var(--text); font: 400 16px/1.65 ui-monospace, "SF Mono", Menlo, Consolas, "Liberation Mono", monospace; }}
a {{ color: var(--ink); text-underline-offset: 3px; }}
a:hover {{ background: var(--ink); color: var(--bg); text-decoration: none; }}
:focus-visible {{ outline: 2px solid var(--ink); outline-offset: 2px; }}
.skip {{ position: absolute; left: 8px; top: -60px; z-index: 5; padding: 6px 10px; background: var(--ink); color: var(--bg); font: 16px/1 var(--pixel); }}
.skip:focus {{ top: 8px; }}
.sr {{ position: absolute; width: 1px; height: 1px; overflow: hidden; clip-path: inset(50%); white-space: nowrap; }}
header {{ position: sticky; top: 0; z-index: 3; display: flex; gap: 14px; align-items: center; flex-wrap: wrap;
  padding: 10px 16px; background: var(--bg); border-bottom: 1px solid var(--line); font: 16px/1 var(--pixel); }}
header .home {{ color: var(--ink); text-decoration: none; margin-right: auto; font-size: 20px; display: inline-flex;
  align-items: center; gap: 10px; }}
header .home svg {{ width: 22px; height: 22px; flex: none; }}
header a {{ color: var(--mid); text-decoration: none; padding: 3px 4px 2px; }}
header a[aria-current] {{ color: var(--bg); background: var(--ink); }}
.theme, .contents {{ font: 16px/1 var(--pixel); color: var(--mid); background: none; border: 0; padding: 3px 4px 2px; cursor: pointer; }}
.theme:hover, .contents:hover {{ color: var(--bg); background: var(--ink); }}
.theme {{ display: inline-flex; align-items: center; gap: 6px; }}
.theme svg {{ width: 15px; height: 15px; flex: none; }}
@media (max-width: 600px) {{ .theme-word {{ display: none; }} }}
.contents {{ display: none; }}
.layout {{ display: grid; grid-template-columns: 300px minmax(0, 1fr); max-width: 1240px; margin: 0 auto; }}
nav.side {{ position: sticky; top: 49px; align-self: start; max-height: calc(100vh - 49px); overflow: auto; padding: 16px 12px 40px 16px;
  border-right: 1px solid var(--line); font-size: 14px; line-height: 1.4; }}
nav.side input {{ width: 100%; font: inherit; color: var(--text); background: var(--surf); border: 1px solid var(--line); padding: 6px 8px; margin-bottom: 12px; }}
.toc, .toc ol {{ list-style: none; margin: 0; padding: 0; }}
.toc > li {{ margin: 0 0 4px; }}
.toc ol {{ display: none; margin: 2px 0 6px 12px; }}
.toc li.open > ol {{ display: block; }}
.toc a {{ display: block; color: var(--mid); text-decoration: none; padding: 2px 4px; }}
.toc a:hover {{ color: var(--bg); }}
.toc li.open > a, .toc a.here {{ color: var(--ink); }}
.toc li.group {{ margin: 16px 0 4px; padding: 0 4px; color: var(--dim); font: 14px/1.2 var(--pixel); }}
.toc li.group:first-child {{ margin-top: 0; }}
.results {{ list-style: none; margin: 0; padding: 0; }}
.results li {{ margin: 0 0 10px; }}
.results small {{ display: block; color: var(--dim); }}
.results mark {{ background: var(--ink); color: var(--bg); }}
main {{ padding: 8px 32px 80px; min-width: 0; max-width: 900px; }}
main li {{ margin: 0 0 10px; }}
main li > p {{ margin: 6px 0; }}
main li > strong:first-child {{ color: var(--ink); }}
main h1, main h2, main h3, main h4 {{ color: var(--ink); font-family: var(--pixel); font-weight: 400; line-height: 1.2; }}
main h1 {{ font-size: clamp(28px, 4vw, 44px); margin: 24px 0 12px; }}
main h2 {{ font-size: clamp(22px, 3vw, 32px); margin: 56px 0 12px; padding-top: 12px; border-top: 1px solid var(--line); }}
main h3 {{ font-size: 20px; margin: 36px 0 8px; }}
main h4 {{ font-size: 16px; margin: 24px 0 6px; }}
main img {{ max-width: 100%; height: auto; image-rendering: pixelated; }}
main p[align="center"] {{ text-align: center; }}
main p[align="center"] img {{ display: inline-block; margin: 4px; }}
main sub {{ color: var(--dim); font-size: 13px; }}
main .videos {{ display: grid; grid-template-columns: repeat(auto-fill, minmax(220px, 1fr)); gap: 12px; margin: 12px 0 20px; }}
main .videos figure {{ margin: 0; }}
main .videos figure.on {{ grid-column: 1 / -1; }}
main .videos figcaption {{ color: var(--dim); font-size: 13px; margin-top: 4px; }}
main .videos .play {{ position: relative; display: block; line-height: 0; border: 1px solid var(--line); }}
main .videos .play:hover, main .videos .play:focus-visible {{ border-color: var(--ink); }}
main .videos .play:focus-visible {{ outline: 2px solid var(--ink); outline-offset: 2px; }}
main .videos .play img {{ width: 100%; image-rendering: auto; }}
main .videos .play::before {{ content: ""; position: absolute; right: 4px; bottom: 4px; width: 36px; height: 24px; background: var(--bg); border: 1px solid var(--ink); }}
main .videos .play::after {{ content: ""; position: absolute; right: 16px; bottom: 10px; border-style: solid; border-width: 6px 0 6px 10px; border-color: transparent transparent transparent var(--ink); }}
main .videos iframe {{ display: block; width: 100%; aspect-ratio: 16 / 9; border: 1px solid var(--ink); }}
main table {{ border-collapse: collapse; display: block; overflow-x: auto; margin: 12px 0; font-size: 14px; }}
main th, main td {{ border: 1px solid var(--line); padding: 6px 10px; text-align: left; vertical-align: top; }}
main th {{ background: var(--surf); color: var(--ink); font-weight: 400; }}
main code {{ background: var(--surf); padding: 1px 4px; }}
main pre {{ background: var(--surf); padding: 12px; overflow-x: auto; }}
main pre code {{ padding: 0; }}
main blockquote {{ margin: 12px 0; padding: 8px 14px; border-left: 4px solid var(--ink); background: var(--surf); }}
main blockquote p {{ margin: 0; }}
main details {{ margin: 12px 0; padding: 6px 14px; border: 1px solid var(--line); }}
main details[open] {{ background: var(--surf); }}
main summary {{ cursor: pointer; color: var(--ink); }}
main hr {{ border: 0; border-top: 1px solid var(--line); margin: 32px 0; }}
main .downloads ul {{ list-style: none; margin: 16px 0; padding: 0; display: grid; gap: 10px; }}
main .downloads li {{ display: grid; grid-template-columns: 110px minmax(0, 1fr) auto; gap: 6px 16px; align-items: center;
  margin: 0; padding: 12px 16px; border: 2px solid var(--line); }}
main .downloads li b {{ color: var(--ink); font: 18px/1.2 var(--pixel); font-weight: 400; }}
main .downloads li > span {{ color: var(--mid); font-size: 14px; }}
main .downloads .get {{ font: 16px/1 var(--pixel); padding: 8px 12px 6px; border: 2px solid var(--ink); text-decoration: none; white-space: nowrap; }}
main .downloads a.get {{ background: var(--ink); color: var(--bg); }}
main .downloads a.get:hover {{ background: var(--bg); color: var(--ink); }}
main .downloads .soon {{ color: var(--dim); border-color: var(--line); }}
main .downloads p {{ color: var(--mid); }}
.next {{ margin-top: 56px; padding-top: 16px; border-top: 1px solid var(--line); }}
@media (max-width: 900px) {{
  .layout {{ display: block; }}
  .contents {{ display: inline-block; }}
  nav.side {{ display: none; position: fixed; top: 49px; left: 0; right: 0; bottom: 0; max-height: none; z-index: 2; background: var(--bg); border: 0; }}
  body.toc-open nav.side {{ display: block; }}
  main {{ padding: 8px 16px 60px; }}
}}
@media (max-width: 560px) {{
  main .downloads li {{ grid-template-columns: minmax(0, 1fr) auto; }}
  main .downloads li > span:not(.get) {{ grid-column: 1 / -1; grid-row: 2; }}
}}
@media (max-width: 600px) {{
  header {{ gap: 6px; }}
  header .home {{ font-size: 18px; }}
  header .wide {{ display: none; }}
}}
</style>
</head>
<body>
<a class="skip" href="#main">Skip to the text</a>
<header>
  <a class="home" href="../" aria-label="Tessera"><svg viewBox="0 0 78 78" aria-hidden="true"><rect x="4" y="5" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="31" y="3" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="53" y="2" width="22" height="22" fill="currentColor"/><rect x="5" y="30" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="30" y="30" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="55" y="31" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="4" y="55" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="31" y="57" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/><rect x="55" y="55" width="18" height="18" fill="none" stroke="currentColor" stroke-width="4"/></svg>TESSERA</a>
  <a href="../start/"{cur_start}>START</a>
  <a href="../manual/"{cur_manual}>MANUAL</a>
  <a href="../plugin/"{cur_plugin}>PLUGIN</a>
  <a class="wide" href="../manual/tessera-manual.pdf" download title="The whole manual as a PDF, to print or read offline">PDF</a>
  <a class="wide" href="{repo}RECIPES.md">RECIPES</a>
  <a class="wide" href="../webapp/sim/">SIMULATOR</a>
  <button class="contents" id="contents" type="button" aria-expanded="false" aria-controls="side">CONTENTS</button>
  <button class="theme" id="theme" type="button" aria-label="Light or dark page"></button>
</header>
<div class="layout">
<nav class="side" id="side" aria-label="Contents">
  <input id="search-q" type="search" placeholder="Search {short}" aria-label="Search {short}" autocomplete="off">
  <p class="sr" id="found" aria-live="polite"></p>
  <ol class="results" id="results" hidden></ol>
  {toc}
</nav>
<main id="main" tabindex="-1">
{intro}{body}
<p class="next">{next}</p>
</main>
</div>
<script>/* the contents: the chapter on screen opened and marked; on a narrow screen, CONTENTS shows them */
(function () {{
  var links = [].slice.call(document.querySelectorAll(".toc a")), byId = {{}};
  links.forEach(function (a) {{ byId[a.getAttribute("href").slice(1)] = a; }});
  var heads = [].slice.call(document.querySelectorAll("main h2[id], main h3[id]")).filter(function (h) {{ return byId[h.id]; }});
  function mark() {{
    var cur = null;
    for (var i = 0; i < heads.length; i++) {{ if (heads[i].getBoundingClientRect().top < 90) cur = heads[i]; else break; }}
    links.forEach(function (a) {{ a.classList.remove("here"); a.parentNode.classList.remove("open"); }});
    if (!cur) return;
    var a = byId[cur.id]; a.classList.add("here");
    var li = a.parentNode; while (li && li.tagName === "LI") {{ li.classList.add("open"); li = li.parentNode.parentNode; }}
  }}
  var busy = false;
  window.addEventListener("scroll", function () {{ if (!busy) {{ busy = true; requestAnimationFrame(function () {{ busy = false; mark(); }}); }} }});
  mark();
  var b = document.getElementById("contents");
  b.addEventListener("click", function () {{ var o = document.body.classList.toggle("toc-open"); b.setAttribute("aria-expanded", o); }});
  document.getElementById("side").addEventListener("click", function (e) {{
    if (e.target.tagName === "A") {{ document.body.classList.remove("toc-open"); b.setAttribute("aria-expanded", "false"); }}
  }});
}})();
/* a table or code block wider than the page scrolls sideways: it then takes the keyboard focus, so the arrow keys
   scroll it */
(function () {{
  var boxes = [].slice.call(document.querySelectorAll("main table, main pre"));
  function fit() {{
    boxes.forEach(function (b) {{ if (b.scrollWidth > b.clientWidth + 1) b.tabIndex = 0; else b.removeAttribute("tabindex"); }});
  }}
  fit(); window.addEventListener("resize", fit);
}})();
/* the search: every section's text (from its heading to the next), the matching ones listed with a line of each */
(function () {{
  var q = document.getElementById("search-q"), out = document.getElementById("results"), found = document.getElementById("found"), toc = document.querySelector(".toc"), secs = [], cur = null;
  [].forEach.call(document.getElementById("main").children, function (el) {{
    if (/^H[1-4]$/.test(el.tagName) && el.id) {{ cur = {{ id: el.id, title: el.textContent, text: "" }}; secs.push(cur); }}
    else if (cur) cur.text += " " + el.textContent;
  }});
  function esc(s) {{ return s.replace(/[&<>"]/g, function (c) {{ return {{ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }}[c]; }}); }}
  q.addEventListener("input", function () {{
    var w = q.value.trim().toLowerCase();
    if (w.length < 2) {{ out.hidden = true; toc.hidden = false; found.textContent = ""; return; }}
    var html = "", n = 0;
    secs.forEach(function (s) {{
      if (n >= 40) return;
      var t = s.text.replace(/\\s+/g, " "), i = t.toLowerCase().indexOf(w), inTitle = s.title.toLowerCase().indexOf(w) >= 0;
      if (i < 0 && !inTitle) return;
      n++;
      var line = i < 0 ? "" : esc(t.slice(Math.max(0, i - 40), i)) + "<mark>" + esc(t.substr(i, w.length)) + "</mark>" + esc(t.substr(i + w.length, 60));
      html += '<li><a href="#' + s.id + '">' + esc(s.title) + "</a>" + (line ? "<small>&hellip;" + line + "&hellip;</small>" : "") + "</li>";
    }});
    out.innerHTML = html || "<li><small>Nothing found.</small></li>";
    found.textContent = n ? n + (n >= 40 ? "+" : "") + (n === 1 ? " section found" : " sections found") : "Nothing found";
    out.hidden = false; toc.hidden = true;
  }});
  q.addEventListener("keydown", function (e) {{ if (e.key === "Escape") {{ q.value = ""; q.dispatchEvent(new Event("input")); }} }});
}})();
/* the guided clips: a thumbnail (a link to YouTube without script) becomes the player when played; nothing is
   fetched from YouTube before that, and the player takes the focus */
(function () {{
  [].forEach.call(document.querySelectorAll("main .videos .play"), function (a) {{
    a.addEventListener("click", function (e) {{
      if (e.ctrlKey || e.metaKey || e.shiftKey) return;
      e.preventDefault();
      var f = document.createElement("iframe"), fig = a.parentNode;
      f.src = "https://www.youtube-nocookie.com/embed/" + a.dataset.video + "?autoplay=1&rel=0";
      f.title = a.getAttribute("aria-label").replace("Play the video: ", "Video: ");
      f.allow = "autoplay; encrypted-media; picture-in-picture; fullscreen";
      f.allowFullscreen = true;
      fig.className = "on";
      fig.replaceChild(f, a);
      f.focus();
    }});
  }});
}})();
/* LIGHT / DARK: names the other look; the choice is stored (tessera-theme), as on the project page */
(function () {{
  var b = document.getElementById("theme"), r = document.documentElement;
  function light() {{ return r.dataset.theme === "light"; }}
  /* the LIGHT / DARK button: the sun or moon of the LANDSCAPE screen (firmware ui_land.c), then the word (hidden on
   * phones, kept as the name) */
  function themeFace(b, word, toLight) {{
    b.innerHTML = '<svg viewBox="0 0 15 15" aria-hidden="true" shape-rendering="crispEdges"><path fill="currentColor" d="'
      + (toLight ? "M7 0h1v2h-1zM3 3h1v1h-1zM5 3h5v1h-5zM11 3h1v1h-1zM4 4h2v1h-2zM9 4h2v1h-2zM3 5h2v1h-2zM10 5h2v1h-2zM3 6h1v3h-1zM11 6h1v3h-1zM0 7h2v1h-2zM13 7h2v1h-2zM3 9h2v1h-2zM10 9h2v1h-2zM4 10h2v1h-2zM9 10h2v1h-2zM3 11h1v1h-1zM5 11h5v1h-5zM11 11h1v1h-1zM7 13h1v2h-1z" : "M5 2h2v1h-2zM4 3h2v1h-2zM3 4h3v1h-3zM2 5h4v3h-4zM2 8h5v1h-5zM2 9h6v1h-6zM3 10h9v1h-9zM4 11h7v1h-7zM5 12h5v1h-5z") + '"/></svg>';
    var s = document.createElement("span"); s.className = "theme-word"; s.textContent = word; b.appendChild(s);
    b.setAttribute("aria-label", word); b.title = word;
  }}
  function show() {{ themeFace(b, light() ? "DARK" : "LIGHT", !light()); }}
  b.addEventListener("click", function () {{
    r.dataset.theme = light() ? "dark" : "light";
    try {{ localStorage.setItem("tessera-theme", r.dataset.theme); }} catch (e) {{}}
    show();
  }});
  show();
}})();
</script>
</body>
</html>
"""

NEXT = {
    "start": 'Next: <a href="../manual/#tessera-in-plain-words">Tessera in plain words</a>, then the whole <a href="../manual/">manual</a>.',
    "manual": 'New to Tessera? Start with <a href="../start/">Getting started</a>. '
              'To print it or read it offline: <a href="tessera-manual.pdf" download>the manual as a PDF</a>.',
    "plugin": 'Everything else, the same in the plugin as on the FM-1: the <a href="../manual/">manual</a>. New to Tessera? '
              'Start with <a href="../start/">Getting started</a>.',
}


def make(out):
    out = Path(out)
    face, dark, light = font_and_colours()
    for src, page in PAGES.items():
        text = videos((ROOT / src).read_text(encoding="utf-8"), "web")
        body, heads = convert(downloads(text) if page == "plugin" else text, page)
        dest = out / page / "index.html"
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(PAGE.format(src=src, title=TITLES[page], face=face, dark=dark, light=light, repo=REPO,
                                    # (a narrow screen has room for two of START, MANUAL and PLUGIN: the plugin's page drops START,
                                    # the others PLUGIN)
                                    cur_start=' aria-current="page"' if page == "start" else ' class="wide"' if page == "plugin" else "",
                                    cur_manual=' aria-current="page"' if page == "manual" else "",
                                    cur_plugin=' aria-current="page"' if page == "plugin" else ' class="wide"',
                                    short={"manual": "the manual", "plugin": "the plugin's guide"}.get(page, "this guide"),
                                    toc=sidebar(heads, page), body=body, next=NEXT[page],
                                    intro=f'<p class="next" style="margin:16px 0 0;border:0;padding:0">{NEXT[page]}</p>' if page == "manual" else ""), encoding="utf-8")
        print(f"make_manual: {dest} ({len(heads)} headings)")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    make(sys.argv[1])
