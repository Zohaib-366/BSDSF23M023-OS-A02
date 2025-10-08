# lsv — Custom `ls` Implementation (Course Project)

**Course:** Operating Systems — Programming Assignment 02  
**Instructor:** Muhammad Arif Butt, PhD  
**Student:** Muhammad Zohaib  
**Repository name:** `ROLL_NO-OS-A02` (replace `ROLL_NO` with your roll number)  
**Final Version:** v1.6.0  
**Language:** C  
**Build system:** Makefile

---

## Table of Contents

- [Project Overview](#project-overview)
- [Features (v1.0.0 → v1.6.0)](#features-v100--v160)
- [Build & Install](#build--install)
- [Usage & Examples](#usage--examples)
- [Command-line Options](#command-line-options)
- [Implementation Notes](#implementation-notes)
- [Testing & Verification](#testing--verification)
- [Git Workflow and Releases](#git-workflow-and-releases)
- [Report & Viva Tips](#report--viva-tips)
- [Known Limitations & Future Work](#known-limitations--future-work)
- [License](#license)
- [Contact](#contact)

---

## Project Overview

This project is a step-by-step re-engineering of the Unix `ls` command for learning purposes. You will find incremental features implemented across multiple versions, each corresponding to assignment milestones (v1.0.0 → v1.6.0). The program demonstrates use of POSIX APIs (opendir/readdir, stat/lstat, ioctl), dynamic memory, sorting (`qsort`), terminal formatting, ANSI colors, and recursion.

The code base intentionally evolves without major structural changes so each new feature extends the previous one.

---

## Features (v1.0.0 → v1.6.0)

**v1.0.0 — Project scaffold**  
- Initial build environment, `Makefile`, `src/` skeleton and a working compile/run pipeline.

**v1.1.0 — Long listing (`-l`)**  
- Displays file metadata (permissions, link count, owner, group, size, modification time) using `lstat()` / `stat()` and `getpwuid()` / `getgrgid()`.

**v1.2.0 — Column display (down then across)**  
- Default output (no `-l`) shows files in multiple aligned columns. Terminal width is detected via `ioctl(TIOCGWINSZ)` with an 80-character fallback.

**v1.3.0 — Horizontal column display (`-x`)**  
- New `-x` flag prints files left-to-right (row-major), wrapping when the line would exceed the terminal width.

**v1.4.0 — Alphabetical sort**  
- Filenames are read into memory and alphabetically sorted using `qsort()` and a `compare` function before rendering. Sorting is case-insensitive (uses `strcasecmp`).

**v1.5.0 — Colorized output**  
- Uses ANSI escape codes to color filenames based on type:
  - Blue — directories
  - Green — executables
  - Red — archives (`.tar`, `.gz`, `.zip`)
  - Pink/Cyan — symbolic links
  - Reverse video — special files (device nodes, sockets, FIFOs)

**v1.6.0 — Recursive listing (`-R`)**  
- Prints directory header and recursively descends into subdirectories (skips `.` and `..`). Proper path construction is used (`parent/child`) before `lstat()` calls.

---

## Build & Install

1. Clone your repository to local machine (example):
   ```bash
   git clone https://github.com/<your-username>/ROLL_NO-OS-A02.git
   cd ROLL_NO-OS-A02
   ```

2. Build using `make`:
   ```bash
   make
   ```
   This produces binary(ies) under `bin/` (e.g. `bin/lsv1.6.0`), and object files in `obj/`.

3. Clean build artifacts:
   ```bash
   make clean
   ```

> If your Makefile and final binary use different names adjust the example commands accordingly.

---

## Usage & Examples

**Basic (current directory):**
```bash
./bin/lsv1.6.0
```

**Long listing:**
```bash
./bin/lsv1.6.0 -l
```

**Horizontal columns (row-major):**
```bash
./bin/lsv1.6.0 -x
```

**Recursive listing:**
```bash
./bin/lsv1.6.0 -R
```

**Combining flags:**
```bash
./bin/lsv1.6.0 -lR         # Long listing and recursive
./bin/lsv1.6.0 -x --color  # Horizontal columns with colors (if available)
```

**Specify a directory (or multiple):**
```bash
./bin/lsv1.6.0 /etc /usr/bin
```

---

## Command-line Options (supported)

- `-l` : Long listing format (show metadata)
- `-x` : Horizontal (across) column layout
- `-r` : Reverse sort order (if implemented)
- `-R` : Recursive directory listing
- `--color` or color by default (v1.5.0) : When color feature enabled, output is colorized based on file type.

> Use combinations to get combined behaviors. The program prioritizes `-l` for long-format display and otherwise selects column display mode based on `-x` or default behavior.

---

## Implementation Notes (important details)

- **Reading directories:** `opendir()` / `readdir()` used to gather filenames. Hidden files (names starting with `.`) are skipped unless `-a` is implemented.
- **Dynamic memory:** Filenames are `strdup()`-ed and stored in a dynamically grown `char **` array via `realloc()`. Always free memory after use.
- **Sorting:** `qsort()` sorts the array. The comparison function uses `strcasecmp` for case-insensitive lexicographic ordering.
- **Column layout math (default):**
  - `col_width = max_filename_length + spacing`
  - `columns = terminal_width / col_width` (at least 1)
  - `rows = ceil(count / columns)`
  - Index mapping for down-then-across: `idx = col * rows + row`
- **Horizontal layout (`-x`):**
  - Print left-to-right, track current printed width; wrap when next column exceeds `terminal_width`.
- **Terminal width detection:** Uses `ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);` fallback to 80 columns if ioctl fails.
- **Colorization:** ANSI escape sequences (e.g. `"\033[0;34m"` for blue) are used. Each colored print must be reset with `"\033[0m"`.
- **File metadata queries:**
  - Use `lstat()` when you need to detect symbolic links (so the link itself is examined, not the target).
  - Use `stat()` when you want the target's metadata.
- **Recursive listing:** Before recursing, construct `path = parent + "/" + entry` (take care for trailing slashes). Skip `.` and `..`.

---

## Testing & Verification

- **Minimal tests:**
  - `./bin/lsv1.6.0` — current directory listing
  - `./bin/lsv1.6.0 -l` — long listing
  - `./bin/lsv1.6.0 -x` — check horizontal layout wraps
  - `./bin/lsv1.6.0 -R src` — recursive listing on a source tree

- **Edge cases:**
  - Empty directories
  - Filenames with spaces or special characters
  - Very long filenames (longer than terminal width)
  - Large directory counts (performance/ memory)
  - Symbolic links pointing to directories vs files
  - File permission errors (lack of `read` permission)

---

## Git Workflow and Releases (recommended)

1. Create feature branch:
   ```bash
   git checkout -b feature-long-listing-v1.1.0
   ```

2. Commit changes with small, descriptive messages:
   ```bash
   git add src/lsv1.1.0.c
   git commit -m "feat(ls): implement long listing -l using lstat()"
   ```

3. Merge to main (after review and testing):
   ```bash
   git checkout main
   git merge --no-ff feature-long-listing-v1.1.0
   ```

4. Tag new release (exact tag names required by assignment):
   ```bash
   git tag -a v1.1.0 -m "Version 1.1.0 - Long Listing"
   git push origin main --tags
   ```

5. Draft a GitHub Release using the annotated tag. Attach compiled binary if requested by assignment.

**Important:** Keep branches for each feature (e.g., `feature-column-display-v1.2.0`, `feature-horizontal-display-v1.3.0`, ...). Make commit messages clear and atomic.

---

## Report & Viva Tips

- **REPORT.md should include:**
  - Explanation of each feature and why it was implemented the way it was.
  - The algorithm for column alignment (math for rows & columns).
  - Explanation of `stat()` vs `lstat()`.
  - Explanation of `qsort()` comparison function signature and why it uses `const void *`.
  - Discussion on memory usage and scalability (reading all entries into memory).
  - Demonstration steps you used to test edge cases.

- **Viva prep:**
  - Be ready to explain any line of code (you must not claim personal experience you don’t have).
  - Explain terminal width detection and its limitations.
  - Explain how color codes work and which `st_mode` bits you used to detect executables.
  - Demonstrate the recursive base case and why `parent/child` path construction is necessary.

---

## Known Limitations & Future Work

- Skips hidden files (`.`-prefixed) by default; implement `-a` to show hidden files.
- No locale-aware sorting or human-readable sizes (could add `-h`).
- Performance: reading very large directories into memory may exhaust RAM; consider streaming + partial sort / external sort for extremely large directories.
- Terminal width calculation may not account for Unicode/ANSI escape sequences length; wide Unicode characters may disrupt column alignment.
- Improve color preferences by allowing `LS_COLORS`-compatible configuration.

---

## License

This project is intended as a student assignment. Use the MIT License if you want to open-source it:

```
MIT License
Copyright (c) 2025 Muhammad Zohaib
Permission is hereby granted...
```

(Include a full LICENSE file in your repo if you publish it.)

---

## Contact

**Instructor:** Muhammad Arif Butt, PhD  
**Student (author):** Muhammad ZohaiB  
**Repository:** `BSDSF23M023-OS-A02` on GitHub

---

**End of README**
