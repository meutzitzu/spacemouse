import os


def print_tree(base_dir, ignore_dirs):
    tree_lines = []
    prefix = ''

    def walk_dir(path, prefix=''):
        entries = sorted(os.listdir(path))
        entries = [e for e in entries if e not in ignore_dirs]
        entries_count = len(entries)
        for i, entry in enumerate(entries):
            full_path = os.path.join(path, entry)
            connector = "└── " if i == entries_count - 1 else "├── "
            tree_lines.append(prefix + connector + entry)
            if os.path.isdir(full_path):
                extension = "    " if i == entries_count - 1 else "│   "
                walk_dir(full_path, prefix + extension)

    tree_lines.append(".")
    walk_dir(base_dir)
    return '\n'.join(tree_lines)

base_dir = ".."
output_md = "code_listing.md"
include_extensions = {'.js', '.html', '.css', '.py', '.cpp', '.hpp', '.ini' }
ignore_dirs = {'.git', '.pio', '.cache', 'docs' }

def should_include(file):
    return os.path.splitext(file)[1] in include_extensions

with open(output_md, "w", encoding="utf-8") as out:

    tree_text = print_tree(base_dir, ignore_dirs)
    out.write("## Project File Tree\n\n```\n")
    out.write(tree_text)
    out.write("\n```\n\\vspace{1cm}")

    for root, dirs, files in os.walk(base_dir):
        dirs[:] = [d for d in dirs if d not in ignore_dirs]
        for file in sorted(files):
            if should_include(file):
                filepath = os.path.join(root, file)
                rel_path = os.path.relpath(filepath, base_dir)
                out.write(f"\n\n## `{rel_path}`\n\n")
                out.write("```" + os.path.splitext(file)[1][1:] + "\n")
                try:
                    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
                        out.write(f.read())
                except Exception as e:
                    out.write(f"ERROR reading file: {e}")
                out.write("\n```\n\\vspace{1.5cm}")
