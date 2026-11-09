// SPDX-License-Identifier: MIT OR Apache-2.0
// SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors

//! cargo-fuzz target over the Rust loader (protocol G5). It uses only the safe core API: create an instance, load the
//! input bytes, step once when the load was accepted, and read the digest. Refusals are the expected outcome for almost
//! every input and are not errors. This file holds no hand-written code that bypasses the compiler safety checks; only the `fuzz_target!` expansion does.
#![no_main]

use duospool_core::{DIGEST_HEX_LEN, Instance, Status};
use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    let mut inst = Instance::new();
    if inst.load(data) == Status::Ok {
        let _ = inst.step(100);
    }
    let mut out = [0_u8; DIGEST_HEX_LEN];
    let _ = inst.digest_hex(&mut out);
});
