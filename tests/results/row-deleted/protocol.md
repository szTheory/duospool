# Sample protocol for the check-results controls

<!-- metric-register:begin -->
| Metric | Variant | Name | Command | Feeds |
|---|---|---|---|---|
| M02 | debug | clean build, warm toolchain | `hyperfine --runs 10 'sh build.sh <opt> debug'` | reported only |
| M08 | sanitizer | sanitizer run over the manifest runner | prose row, no command | C1, C5 |
<!-- metric-register:end -->
