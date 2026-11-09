# Control register: stale tracked entry warns but passes

## Entries
id: PROV-0001
source-url: https://example.invalid/upstream/repo
upstream-ref: 0123456789abcdef0123456789abcdef01234567
upstream-path: src/helper.c lines 1-20
license-spdx: MIT
license-url: https://example.invalid/upstream/repo/blob/0123456789abcdef0123456789abcdef01234567/LICENSE
upstream-notice: Copyright (c) Example Author
local-paths: src/copied.c
modifications: none
copied-on: 2024-01-15
copied-by: The DuoSpool Authors
maintenance-owner: owner
why-copy-not-dependency: control fixture only, no real code
sync-policy: tracked
licence-decision: control fixture only, no real code
