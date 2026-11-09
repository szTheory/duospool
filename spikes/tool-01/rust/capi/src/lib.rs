// SPDX-License-Identifier: MIT OR Apache-2.0
// SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors

//! The extern "C" facade of the TOOL-01 spike workload (protocol W-04, W-10). It wraps the safe `duospool_core` and is the
//! only crate in this skeleton that contains hand-written `unsafe`. Every unsafe block has a SAFETY comment. Every path
//! returns a status; the workspace sets `panic = "abort"` as the backstop, so no panic can unwind across the ABI.
//! The matching header is `include/duospool.h`, which is byte-identical to the C skeleton's.

use duospool_core::{DIGEST_HEX_LEN, Instance, Status, length_status};
use std::ffi::c_char;
use std::ptr;

/// The opaque instance behind the C `ds_instance` type.
pub struct Handle {
    inst: Instance,
}

#[unsafe(no_mangle)]
pub extern "C" fn duospool_abi_version() -> u32 {
    1
}

/// # Safety
/// `out` must be null or valid for a write of one pointer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn duospool_create(out: *mut *mut Handle) -> u32 {
    if out.is_null() {
        return Status::ErrNullArg.code();
    }
    let handle = Box::new(Handle {
        inst: Instance::new(),
    });
    // SAFETY: `out` is non-null and the caller guarantees it is valid for a write of one pointer.
    unsafe { *out = Box::into_raw(handle) };
    Status::Ok.code()
}

/// # Safety
/// `inst` must be null or a live pointer from `duospool_create` with no other reference in use. `buf` must be null or
/// valid for reads of `len` bytes; it is not read when `len` is refused by length alone.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn duospool_load(inst: *mut Handle, buf: *const u8, len: usize) -> u32 {
    if inst.is_null() || buf.is_null() {
        return Status::ErrNullArg.code();
    }
    // Decided from the length alone: for a refused length no slice is formed and no byte of `buf` is read (W-05).
    let by_length = length_status(len);
    if by_length != Status::Ok {
        return by_length.code();
    }
    // SAFETY: `inst` is non-null and, by the contract above, a live pointer from `duospool_create` with no other
    // reference in use.
    let handle = unsafe { &mut *inst };
    // SAFETY: `buf` is non-null, `length_status(len)` is Ok so `len` is exactly the 65536 content bytes, and the
    // caller guarantees `buf` is valid for reads of `len` bytes.
    let bytes = unsafe { std::slice::from_raw_parts(buf, len) };
    handle.inst.load(bytes).code()
}

/// # Safety
/// `inst` must be null or a live pointer from `duospool_create` with no other reference in use.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn duospool_step(inst: *mut Handle, cycles: u32) -> u32 {
    if inst.is_null() {
        return Status::ErrNullArg.code();
    }
    // SAFETY: `inst` is non-null and, by the contract above, a live pointer with no other reference in use.
    let handle = unsafe { &mut *inst };
    handle.inst.step(cycles).code()
}

/// # Safety
/// `inst` must be null or a live pointer from `duospool_create`. `out` must be null or valid for a write of a `u64`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn duospool_time(inst: *const Handle, out: *mut u64) -> u32 {
    if inst.is_null() || out.is_null() {
        return Status::ErrNullArg.code();
    }
    // SAFETY: `inst` is non-null and, by the contract above, a live pointer from `duospool_create`.
    let handle = unsafe { &*inst };
    // SAFETY: `out` is non-null and the caller guarantees it is valid for a write of a `u64`.
    unsafe { *out = handle.inst.time() };
    Status::Ok.code()
}

/// # Safety
/// `inst` must be null or a live pointer from `duospool_create`. `out` must be null or valid for writes of `out_len`
/// bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn duospool_digest_hex(
    inst: *const Handle,
    out: *mut c_char,
    out_len: usize,
) -> u32 {
    if inst.is_null() || out.is_null() {
        return Status::ErrNullArg.code();
    }
    if out_len < DIGEST_HEX_LEN {
        return Status::ErrShortBuffer.code();
    }
    // SAFETY: `inst` is non-null and, by the contract above, a live pointer from `duospool_create`.
    let handle = unsafe { &*inst };
    // SAFETY: `out` is non-null and `out_len >= DIGEST_HEX_LEN`, so the caller guarantees the first `DIGEST_HEX_LEN`
    // bytes are valid for writes; only that prefix is borrowed, whatever the full `out_len` is.
    let bytes = unsafe { std::slice::from_raw_parts_mut(out.cast::<u8>(), DIGEST_HEX_LEN) };
    handle.inst.digest_hex(bytes).code()
}

/// # Safety
/// `inst` must be null or a pointer from `duospool_create` that has not been destroyed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn duospool_destroy(inst: *mut Handle) {
    if inst.is_null() {
        return;
    }
    // SAFETY: `inst` is non-null and, by the contract above, came from `Box::into_raw` in `duospool_create` and has not
    // been destroyed, so ownership returns to the Box exactly once.
    drop(unsafe { Box::from_raw(inst) });
}

/// Safe probes for the cases that only exist at the C boundary. The manifest runner forbids unsafe code, so it reaches
/// the null-buffer and short-buffer cases through these functions, and all the unsafe code stays in this crate.
pub mod probe {
    use super::*;

    /// Create an instance, call `duospool_load` with a null buffer and the content length, destroy the instance and
    /// return the status code.
    pub fn probe_load_null() -> u32 {
        let mut inst: *mut Handle = ptr::null_mut();
        // SAFETY: `&mut inst` is a valid, aligned pointer to a local pointer that lives for the whole call.
        let created = unsafe { duospool_create(&mut inst) };
        if created != Status::Ok.code() {
            return created;
        }
        // SAFETY: `inst` is the live instance created above. A null `buf` is the case under test and is checked
        // before any read.
        let status = unsafe { duospool_load(inst, ptr::null(), duospool_core::CONTENT_SIZE) };
        // SAFETY: `inst` came from `duospool_create` and is destroyed exactly once, here.
        unsafe { duospool_destroy(inst) };
        status
    }

    /// Create an instance, call `duospool_digest_hex` into a heap buffer of exactly `out_len` bytes, destroy the
    /// instance and return the status code. A length below 17 gives `DS_ERR_SHORT_BUFFER`.
    pub fn probe_digest_short(out_len: usize) -> u32 {
        let mut inst: *mut Handle = ptr::null_mut();
        // SAFETY: `&mut inst` is a valid, aligned pointer to a local pointer that lives for the whole call.
        let created = unsafe { duospool_create(&mut inst) };
        if created != Status::Ok.code() {
            return created;
        }
        let mut buf = vec![0_u8; out_len];
        // SAFETY: `inst` is the live instance created above, and `buf` is valid for writes of exactly `out_len` bytes
        // (a zero-length Vec has a dangling but non-null pointer, which is never written when `out_len` is short).
        let status =
            unsafe { duospool_digest_hex(inst, buf.as_mut_ptr().cast::<c_char>(), out_len) };
        // SAFETY: `inst` came from `duospool_create` and is destroyed exactly once, here.
        unsafe { duospool_destroy(inst) };
        status
    }
}
