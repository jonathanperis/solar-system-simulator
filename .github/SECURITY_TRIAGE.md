# Native CLI file-access review

The simulator and headless runner are local applications using the invoking user's filesystem permissions. `--experiment FILE`, `--compare FILE`, and `--output FILE` intentionally select local files. These paths are not accepted by a network service and the executables do not establish an elevated-privilege file-access boundary.

CodeQL path-injection findings #12–15 follow these explicit CLI selections into their file operations. They are false positives for privilege escalation under this contract. Preserve user-selected file access; reassess the findings if these operations become a privileged service or consume paths automatically from remote inputs. No query-wide suppression is used.

Native CSV creation uses an explicit owner-read/write mode on POSIX instead of inheriting permissive creation permissions from `fopen`. Existing-file permissions retain normal OS open semantics. The browser export continues using its temporary virtual-filesystem stream. A CLI test verifies creation under umask zero.

Catalog import names are bounded by the parser and copied into session-owned buffers through a size-aware operation. Tests cover the maximum accepted name, rejected overlong input, ownership across reset, and preservation of the prior session on rejection.
