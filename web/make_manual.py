#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the site's manual pages (GitHub Pages), from the repository's markdown:
  manual/index.html   MANUAL.md
  start/index.html    GETTING_STARTED.md

Each page: the project page's look (Tessera Mono and the colours from web/home/index.html, the light / dark choice it
stores, tessera-theme), a sidebar of the chapters (the ## headings, the open one's ### under it) and a search over the
page's text, with no network. Headings keep GitHub's anchors (lower case, punctuation dropped, spaces as hyphens, -1
-2 on repeats), so a link to MANUAL.md#a-section on GitHub works here as manual/#a-section.

Links: MANUAL.md and GETTING_STARTED.md to these pages; every other file of the repository (RECIPES.md, docs/...) to
it on GitHub; the pictures (assets/gallery, assets/brand) to the site's media/ (web/make_site.py copies them there).

  web/make_manual.py OUT_DIR            (web/make_site.py calls it; needs markdown-it-py: pip install markdown-it-py)
"""
import html
import re
import sys
from pathlib import Path

from markdown_it import MarkdownIt

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
REPO = "https://github.com/Rastapopolix/tessera/blob/HEAD/"
PAGES = {"MANUAL.md": "manual", "GETTING_STARTED.md": "start"}
# CommonMark, as GitHub reads it (a list item's lines indented by two, raw HTML kept), with its tables
MD = MarkdownIt("commonmark", {"html": True}).enable("table")
TITLES = {"manual": "Tessera manual", "start": "Getting started with Tessera"}


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
    for src, dst in (("assets/gallery/", "media/gallery/"), ("assets/brand/", "media/brand/")):
        if path.startswith(src):
            return "../" + dst + path[len(src):]
    return REPO + path + (f"#{frag}" if frag else "")


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


def sidebar(heads):
    """the chapters (##), each with its sections (###), folded but for the open one (the script opens it)"""
    out, open_ = [], False
    for level, hid, text in heads:
        if level == 2:
            if open_:
                out.append("</ol></li>")
            out.append(f'<li><a href="#{hid}">{html.escape(text)}</a><ol>')
            open_ = True
        elif level == 3 and open_:
            out.append(f'<li><a href="#{hid}">{html.escape(text)}</a></li>')
    if open_:
        out.append("</ol></li>")
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
header {{ position: sticky; top: 0; z-index: 3; display: flex; gap: 14px; align-items: center; flex-wrap: wrap;
  padding: 10px 16px; background: var(--bg); border-bottom: 1px solid var(--line); font: 16px/1 var(--pixel); }}
header .home {{ color: var(--ink); text-decoration: none; margin-right: auto; font-size: 20px; }}
header a {{ color: var(--mid); text-decoration: none; padding: 3px 4px 2px; }}
header a[aria-current] {{ color: var(--bg); background: var(--ink); }}
.theme, .contents {{ font: 16px/1 var(--pixel); color: var(--mid); background: none; border: 0; padding: 3px 4px 2px; cursor: pointer; }}
.theme:hover, .contents:hover {{ color: var(--bg); background: var(--ink); }}
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
main table {{ border-collapse: collapse; display: block; overflow-x: auto; margin: 12px 0; font-size: 14px; }}
main th, main td {{ border: 1px solid var(--line); padding: 6px 10px; text-align: left; vertical-align: top; }}
main th {{ background: var(--surf); color: var(--ink); font-weight: 400; }}
main code {{ background: var(--surf); padding: 1px 4px; }}
main pre {{ background: var(--surf); padding: 12px; overflow-x: auto; }}
main pre code {{ padding: 0; }}
main blockquote {{ margin: 12px 0; padding: 8px 14px; border-left: 4px solid var(--ink); background: var(--surf); }}
main blockquote p {{ margin: 0; }}
main hr {{ border: 0; border-top: 1px solid var(--line); margin: 32px 0; }}
.next {{ margin-top: 56px; padding-top: 16px; border-top: 1px solid var(--line); }}
@media (max-width: 900px) {{
  .layout {{ display: block; }}
  .contents {{ display: inline-block; }}
  nav.side {{ display: none; position: fixed; top: 49px; left: 0; right: 0; bottom: 0; max-height: none; z-index: 2; background: var(--bg); border: 0; }}
  body.toc-open nav.side {{ display: block; }}
  main {{ padding: 8px 16px 60px; }}
}}
@media (max-width: 600px) {{
  header {{ gap: 6px; }}
  header .home {{ font-size: 18px; }}
  header .wide {{ display: none; }}
}}
</style>
</head>
<body>
<header>
  <a class="home" href="../">TESSERA</a>
  <a href="../start/"{cur_start}>START</a>
  <a href="../manual/"{cur_manual}>MANUAL</a>
  <a class="wide" href="{repo}RECIPES.md">RECIPES</a>
  <a class="wide" href="../webapp/sim/">SIMULATOR</a>
  <button class="contents" id="contents" type="button" aria-expanded="false" aria-controls="side">CONTENTS</button>
  <button class="theme" id="theme" type="button" aria-label="Light or dark page"></button>
</header>
<div class="layout">
<nav class="side" id="side" aria-label="Contents">
  <input id="q" type="search" placeholder="Search {short}" aria-label="Search {short}" autocomplete="off">
  <ol class="results" id="results" hidden></ol>
  {toc}
</nav>
<main id="main">
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
/* the search: every section's text (from its heading to the next), the matching ones listed with a line of each */
(function () {{
  var q = document.getElementById("q"), out = document.getElementById("results"), toc = document.querySelector(".toc"), secs = [], cur = null;
  [].forEach.call(document.getElementById("main").children, function (el) {{
    if (/^H[1-4]$/.test(el.tagName) && el.id) {{ cur = {{ id: el.id, title: el.textContent, text: "" }}; secs.push(cur); }}
    else if (cur) cur.text += " " + el.textContent;
  }});
  function esc(s) {{ return s.replace(/[&<>"]/g, function (c) {{ return {{ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }}[c]; }}); }}
  q.addEventListener("input", function () {{
    var w = q.value.trim().toLowerCase();
    if (w.length < 2) {{ out.hidden = true; toc.hidden = false; return; }}
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
    out.hidden = false; toc.hidden = true;
  }});
  q.addEventListener("keydown", function (e) {{ if (e.key === "Escape") {{ q.value = ""; q.dispatchEvent(new Event("input")); }} }});
}})();
/* LIGHT / DARK: names the other look; the choice is stored (tessera-theme), as on the project page */
(function () {{
  var b = document.getElementById("theme"), r = document.documentElement;
  function light() {{ return r.dataset.theme === "light"; }}
  function show() {{ b.textContent = light() ? "DARK" : "LIGHT"; }}
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
    "manual": 'New to Tessera? Start with <a href="../start/">Getting started</a>.',
}


def make(out):
    out = Path(out)
    face, dark, light = font_and_colours()
    for src, page in PAGES.items():
        body, heads = convert((ROOT / src).read_text(encoding="utf-8"), page)
        dest = out / page / "index.html"
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(PAGE.format(src=src, title=TITLES[page], face=face, dark=dark, light=light, repo=REPO,
                                    cur_start=' aria-current="page"' if page == "start" else "",
                                    cur_manual=' aria-current="page"' if page == "manual" else "",
                                    short="the manual" if page == "manual" else "this guide",
                                    toc=sidebar(heads), body=body, next=NEXT[page],
                                    intro=f'<p class="next" style="margin:16px 0 0;border:0;padding:0">{NEXT[page]}</p>' if page == "manual" else ""), encoding="utf-8")
        print(f"make_manual: {dest} ({len(heads)} headings)")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    make(sys.argv[1])
