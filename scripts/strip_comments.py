#!/usr/bin/env python3
"""Remove C/C++ comments and collapse excess blank lines. Respects strings and char literals."""
import re
import sys
from pathlib import Path


def strip_comments(content: str) -> str:
    out = []
    i = 0
    n = len(content)
    in_block_comment = False
    in_line_comment = False
    in_double_string = False
    in_single_string = False
    escape = False
    line_start = True
    last_was_newline = False

    while i < n:
        if escape:
            if not (in_block_comment or in_line_comment):
                out.append(content[i])
            escape = False
            i += 1
            continue

        c = content[i]
        c2 = content[i:i+2] if i + 1 < n else c

        if in_line_comment:
            if c == '\n':
                out.append(c)
                in_line_comment = False
                last_was_newline = True
            i += 1
            continue

        if in_block_comment:
            if c2 == '*/':
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue

        if in_double_string:
            if c == '\\':
                escape = True
                out.append(c)
            elif c == '"':
                in_double_string = False
                out.append(c)
            else:
                out.append(c)
            i += 1
            continue

        if in_single_string:
            if c == '\\':
                escape = True
                out.append(c)
            elif c == "'":
                in_single_string = False
                out.append(c)
            else:
                out.append(c)
            i += 1
            continue

        if c2 == '//':
            in_line_comment = True
            while out and out[-1] in ' \t':
                out.pop()
            i += 2
            continue

        if c2 == '/*':
            in_block_comment = True
            while out and out[-1] in ' \t':
                out.pop()
            i += 2
            continue

        if c == '"':
            in_double_string = True
            out.append(c)
        elif c == "'":
            in_single_string = True
            out.append(c)
        elif c == '\n':
            if last_was_newline:
                pass
            else:
                out.append(c)
            last_was_newline = True
        else:
            if c not in ' \t':
                last_was_newline = False
            out.append(c)
        i += 1

    return ''.join(out)


def collapse_blank_lines(content: str) -> str:
    return re.sub(r'\n{3,}', '\n\n', content).strip() + '\n'


def process_file(path: Path) -> None:
    text = path.read_text(encoding='utf-8', errors='replace')
    text = strip_comments(text)
    text = collapse_blank_lines(text)
    path.write_text(text, encoding='utf-8')


def main():
    root = Path(__file__).resolve().parent.parent
    dirs = [root / 'include', root / 'src', root / 'test']
    for d in dirs:
        if not d.exists():
            continue
        for path in d.rglob('*'):
            if path.suffix not in ('.cpp', '.c', '.h', '.hpp'):
                continue
            try:
                path.relative_to(root / 'lib')
                continue
            except ValueError:
                pass
            process_file(path)
            print(path.relative_to(root))


if __name__ == '__main__':
    main()
