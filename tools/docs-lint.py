#!/usr/bin/env python3
"""Checks the manual against the mechanical rules in docs/STYLE.md: sentence length, semicolons and the words to
avoid. Words in double quotes are mentioned, not used, so the word check skips them.

Usage: tools/docs-lint.py [file.md ...]   (no files: the pages in PAGES)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Every page of the manual, each from the heading given (None: the whole page). The README's top is Vic's own voice,
# so its check starts below it, and development.md is for contributors, as DESIGN.md is.
PAGES = {'README.md': '## In a patch',
         **{f'docs/{p.name}': None for p in sorted((ROOT / 'docs').glob('*.md')) if p.name != 'development.md'}}

STEP_WORDS, TEXT_WORDS = 20, 25

AVOID = {
    r'\bpick(s|ed|ing)? (it |them |a step |the step )?up\b': 'latch',
    r'\bknobs?\b': 'dial',
    r'\bbus\b': 'the row',
    r'\bturn(s|ed|ing)? (it )?up\b': 'raise',
    r'\bcom(e|es|ing) back\b': 'return',
    r'\bset(s|ting)? up\b': 'set, or prepare',
}

MARKUP = [
    (re.compile(r'<picture>.*?</picture>', re.S), ''),
    (re.compile(r'<[^>]+>'), ''),
    (re.compile(r'!\[[^\]]*\]\([^)]*\)'), ''),
    (re.compile(r'\[([^\]]*)\]\([^)]*\)'), r'\1'),
    (re.compile(r'`[^`]*`'), 'code'),
    (re.compile(r'[*_]{1,2}'), ''),
]

SENTENCE_END = re.compile(r'(?<=[.!?])\s+(?=[A-Z"(*\d])')
ABBREVIATIONS = re.compile(r'\b(e\.g|i\.e|etc|vs|approx)\.', re.I)


def plain(text):
    for pattern, repl in MARKUP:
        text = pattern.sub(repl, text)
    return ABBREVIATIONS.sub(lambda m: m.group(1), text)


def blocks(lines):
    """Yields (line number, kind, text) for each paragraph, list item and table cell outside code blocks, in quotes too."""
    fenced, start, buf, kind = False, 0, [], 'text'

    def flush():
        if buf:
            yield start, kind, ' '.join(buf)
        buf.clear()

    for n, line in enumerate(lines, 1):
        if line.lstrip().startswith('```'):
            fenced = not fenced
            yield from flush()
            continue
        stripped = re.sub(r'^(>\s?)+', '', line.strip()).strip()   # a quote's lines are read as any others
        if fenced or not stripped or stripped.startswith('#'):
            yield from flush()
            continue
        if stripped.startswith('|'):
            yield from flush()
            if not re.fullmatch(r'[|\s:-]+', stripped):
                for cell in stripped.strip('|').split('|'):
                    if cell.strip().endswith('.'):   # a cell without a full stop is a label or a list of names
                        yield n, 'cell', cell
            continue
        item = re.match(r'(\d+\.|[-*])\s+(.*)', stripped)
        if item or not buf:
            yield from flush()
            start, kind = n, 'step' if item and item.group(1)[0].isdigit() else 'text'
            stripped = item.group(2) if item else stripped
        buf.append(stripped)
    yield from flush()


def lint(path, start=None):
    lines = path.read_text(encoding='utf-8').splitlines()
    skip = lines.index(start) if start else 0
    problems = []
    for n, kind, block in blocks(lines[skip:]):
        n += skip
        words = plain(block)
        if ';' in words:
            problems.append((n, 'semicolon'))
        unquoted = re.sub(r'"[^"]*"', '', words)
        for pattern, use in AVOID.items():
            for m in re.finditer(pattern, unquoted, re.I):
                problems.append((n, f'"{m.group(0)}": use {use}'))
        limit = STEP_WORDS if kind == 'step' else TEXT_WORDS
        for sentence in SENTENCE_END.split(words):
            count = len(re.findall(r"[\w'’+/-]+", sentence))
            if count > limit:
                problems.append((n, f'{count} words (at most {limit}): "{sentence[:60]}…"'))
    return problems


def main(args):
    files = {Path(a): None for a in args} or {ROOT / p: start for p, start in PAGES.items()}
    total = 0
    for path, start in files.items():
        for n, problem in lint(path, start):
            print(f'{path.relative_to(ROOT) if path.is_absolute() else path}:{n}: {problem}')
            total += 1
    print(f'{total} problem(s) in {len(files)} file(s)', file=sys.stderr)
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
