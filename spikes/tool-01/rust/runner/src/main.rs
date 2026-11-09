// SPDX-License-Identifier: MIT OR Apache-2.0
// SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors

//! The manifest runner of the Rust skeleton (protocol W-08, W-09).
//! usage: runner <fixtures.manifest> <out-dir>
//!
//! It executes every `case` row of the manifest and prints one transcript line per case,
//! `<id> <observed> PASS|FAIL|CONTROL-REJECTED|CONTROL-BROKEN`, then
//! `RUNNER pass=<n> fail=<m> controls_rejected=<k>`, and exits non-zero when any case FAILs or a control is
//! CONTROL-BROKEN. The load, step-digest, refuse-keeps-state and interleave cases use the safe core API. The api-null and
//! api-short cases only exist at the C boundary, so they go through the safe probe functions of the capi crate.
#![forbid(unsafe_code)]

use duospool_core::{DIGEST_HEX_LEN, Instance, Status};
use std::fs;
use std::path::Path;
use std::process::ExitCode;

struct ManifestCase {
    id: String,
    kind: String,
    arg: String,
    expect: String,
    control: bool,
}

fn status_name(code: u32) -> &'static str {
    Status::from_code(code).map_or("ERR_UNKNOWN", Status::name)
}

fn read_fixture(dir: &Path, name: &str) -> Option<Vec<u8>> {
    fs::read(dir.join(name)).ok()
}

fn digest_string(inst: &Instance) -> Option<String> {
    let mut out = [0_u8; DIGEST_HEX_LEN];
    if inst.digest_hex(&mut out) != Status::Ok {
        return None;
    }
    String::from_utf8(out[..DIGEST_HEX_LEN - 1].to_vec()).ok()
}

/// Load `fixture` into a fresh instance and report the status name.
fn run_load(dir: &Path, fixture: &str) -> String {
    let Some(bytes) = read_fixture(dir, fixture) else {
        return "FIXTURE_UNREADABLE".to_string();
    };
    let mut inst = Instance::new();
    inst.load(&bytes).name().to_string()
}

/// Load lcg-64k.bin, record time and digest, attempt hdr-66048.bin, and require the refusal to change nothing.
fn run_keeps_state(dir: &Path) -> String {
    let (Some(good), Some(bad)) = (
        read_fixture(dir, "lcg-64k.bin"),
        read_fixture(dir, "hdr-66048.bin"),
    ) else {
        return "FIXTURE_UNREADABLE".to_string();
    };
    let mut inst = Instance::new();
    if inst.load(&good) != Status::Ok {
        return "SETUP_FAILED".to_string();
    }
    let _ = inst.step(7);
    let (t1, d1) = (inst.time(), digest_string(&inst));
    let refusal = inst.load(&bad);
    let (t2, d2) = (inst.time(), digest_string(&inst));
    if refusal == Status::RefuseHeader && t1 == t2 && d1 == d2 {
        "UNCHANGED".to_string()
    } else {
        "CHANGED".to_string()
    }
}

/// Load the fixture, apply the W-11 steps 100, 200, 300 and report `digest:<hex>`.
fn run_step_digest(dir: &Path, fixture: &str) -> String {
    let Some(bytes) = read_fixture(dir, fixture) else {
        return "FIXTURE_UNREADABLE".to_string();
    };
    let mut inst = Instance::new();
    let loaded = inst.load(&bytes);
    if loaded != Status::Ok {
        return loaded.name().to_string();
    }
    for cycles in [100, 200, 300] {
        if inst.step(cycles) != Status::Ok {
            return "STEP_FAILED".to_string();
        }
    }
    digest_string(&inst).map_or_else(|| "STEP_FAILED".to_string(), |hex| format!("digest:{hex}"))
}

/// W-09: instances A and B load lcg-64k.bin and step 1, 7, 64, 1000 (x100), alternating A and B per step. Both final
/// digests and times must equal a solo instance's.
fn run_interleave(dir: &Path) -> String {
    const CYCLES: [u32; 4] = [1, 7, 64, 1000];
    let Some(bytes) = read_fixture(dir, "lcg-64k.bin") else {
        return "FIXTURE_UNREADABLE".to_string();
    };
    let mut a = Instance::new();
    let mut b = Instance::new();
    let mut solo = Instance::new();
    for inst in [&mut a, &mut b, &mut solo] {
        if inst.load(&bytes) != Status::Ok {
            return "SETUP_FAILED".to_string();
        }
    }
    for _round in 0..100 {
        for cycles in CYCLES {
            if a.step(cycles) != Status::Ok
                || b.step(cycles) != Status::Ok
                || solo.step(cycles) != Status::Ok
            {
                return "SETUP_FAILED".to_string();
            }
        }
    }
    let same = |x: &Instance| digest_string(x) == digest_string(&solo) && x.time() == solo.time();
    if same(&a) && same(&b) {
        "EQUAL".to_string()
    } else {
        "DIFFER".to_string()
    }
}

/// Parse the `case` rows of the manifest: `case <id> <kind> <arg> <expect> [control]`.
fn load_cases(path: &str) -> Option<Vec<ManifestCase>> {
    let text = fs::read_to_string(path).ok()?;
    let mut cases = Vec::new();
    for line in text.lines() {
        let fields: Vec<&str> = line.split_whitespace().collect();
        if fields.len() < 5 || fields[0] != "case" {
            continue;
        }
        cases.push(ManifestCase {
            id: fields[1].to_string(),
            kind: fields[2].to_string(),
            arg: fields[3].to_string(),
            expect: fields[4].to_string(),
            control: fields.get(5).is_some_and(|f| *f == "control"),
        });
    }
    Some(cases)
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 3 {
        eprintln!("usage: runner <fixtures.manifest> <out-dir>");
        return ExitCode::from(2);
    }
    let cases = match load_cases(&args[1]) {
        Some(cases) if !cases.is_empty() => cases,
        _ => {
            eprintln!("runner: no cases in {}", args[1]);
            return ExitCode::from(2);
        }
    };
    let dir = Path::new(&args[2]);
    let (mut pass, mut fail, mut rejected) = (0_u32, 0_u32, 0_u32);
    for case in &cases {
        let observed = match case.kind.as_str() {
            "load" => run_load(dir, &case.arg),
            "api-null" => status_name(duospool::probe::probe_load_null()).to_string(),
            "api-short" => status_name(duospool::probe::probe_digest_short(16)).to_string(),
            "refuse-keeps-state" => run_keeps_state(dir),
            "step-digest" => run_step_digest(dir, &case.arg),
            "interleave" => run_interleave(dir),
            _ => "UNKNOWN_KIND".to_string(),
        };
        let matches = observed == case.expect;
        let verdict = match (case.control, matches) {
            (true, true) => {
                fail += 1;
                "CONTROL-BROKEN"
            }
            (true, false) => {
                rejected += 1;
                "CONTROL-REJECTED"
            }
            (false, true) => {
                pass += 1;
                "PASS"
            }
            (false, false) => {
                fail += 1;
                "FAIL"
            }
        };
        println!("{} {observed} {verdict}", case.id);
    }
    println!("RUNNER pass={pass} fail={fail} controls_rejected={rejected}");
    if fail == 0 {
        ExitCode::SUCCESS
    } else {
        ExitCode::FAILURE
    }
}
