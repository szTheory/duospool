# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# rights-rules.awk - the line rules of scripts/rights-check (FIX-05, D-56, D-76). POSIX awk only. These rules apply to every file,
# text files included: the shape rule is the only one that a valid text file is exempt from.
#   LINELEN  a line longer than LINEMAX bytes (default 4096; a planner threshold above the 1615-byte longest prose line seen at
#            planning time)
#   BLOB     a run of BLOBMIN or more (default 1024) base64 or hex characters inside a line, so a prefix or suffix cannot hide it
#   MAGIC    a bzip2 stream header ("BZh" and a digit 1-9) at the start of the file; the other archive magics are read with od by
#            the caller because they contain bytes that a text file cannot hold
# in : file names as arguments, each prefixed with ./ (so that a name such as a=b is not taken for an assignment), or one stream
#      on stdin with -v NAME=path. Characters are bytes: run with LC_ALL=C.
# out: FILE<TAB>ROMLIKE_BINARY<TAB>LINE<TAB>REASON , at most one LINELEN and one BLOB line per file
BEGIN {
  if (LINEMAX == "") LINEMAX = 4096
  if (BLOBMIN == "") BLOBMIN = 1024
}
function emit(reason, ln) { printf "%s\tROMLIKE_BINARY\t%d\t%s\n", name, ln, reason }
FNR == 1 {
  if (NAME != "") name = NAME
  else { name = FILENAME; sub(/^\.\//, "", name) }
  seen_len = 0
  seen_blob = 0
  if ($0 ~ /^BZh[1-9]/) emit("MAGIC bzip2", 1)
}
{
  n = length($0)
  if (n > LINEMAX + 0 && !seen_len) { emit("LINELEN " n, FNR); seen_len = 1 }
  if (n >= BLOBMIN + 0 && !seen_blob) {
    s = $0
    while (match(s, /[A-Za-z0-9+\/=]+/)) {
      if (RLENGTH >= BLOBMIN + 0) { emit("BLOB", FNR); seen_blob = 1; break }
      s = substr(s, RSTART + RLENGTH)
    }
  }
}
