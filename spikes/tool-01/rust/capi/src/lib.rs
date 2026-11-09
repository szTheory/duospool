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

/// The Miri target (protocol M08, D-36): the facade's own tests drive the `extern "C"` entry points, so the interpreter
/// checks every unsafe block above for undefined behaviour. `cargo +nightly-2026-11-09 miri test -p duospool` runs them.
#[cfg(test)]
mod tests {
    use super::*;
    use duospool_core::CONTENT_SIZE;

    /// A deterministic 65536-byte buffer, built in memory (no fixture file and no host I/O under Miri).
    fn content() -> Vec<u8> {
        (0..CONTENT_SIZE)
            .map(|i| (i.wrapping_mul(31).wrapping_add(7) & 0xff) as u8)
            .collect()
    }

    fn create() -> *mut Handle {
        let mut inst: *mut Handle = ptr::null_mut();
        // SAFETY: `&mut inst` is a valid, aligned pointer to a local pointer that lives for the whole call.
        let status = unsafe { duospool_create(&mut inst) };
        assert_eq!(status, Status::Ok.code());
        assert!(!inst.is_null());
        inst
    }

    #[test]
    fn create_load_step_digest_destroy() {
        let inst = create();
        let buf = content();
        // SAFETY: `inst` is the live instance from `create`, and `buf` is valid for reads of `buf.len()` bytes.
        let status = unsafe { duospool_load(inst, buf.as_ptr(), buf.len()) };
        assert_eq!(status, Status::Ok.code());
        // SAFETY: `inst` is live and not otherwise borrowed.
        assert_eq!(unsafe { duospool_step(inst, 100) }, Status::Ok.code());
        let mut time = 0_u64;
        // SAFETY: `inst` is live and `&mut time` is valid for a write of a `u64`.
        assert_eq!(unsafe { duospool_time(inst, &mut time) }, Status::Ok.code());
        assert_eq!(time, 100);
        let mut hex = [0_u8; DIGEST_HEX_LEN];
        // SAFETY: `inst` is live and `hex` is valid for writes of `DIGEST_HEX_LEN` bytes.
        let status =
            unsafe { duospool_digest_hex(inst, hex.as_mut_ptr().cast::<c_char>(), hex.len()) };
        assert_eq!(status, Status::Ok.code());
        assert_eq!(hex[DIGEST_HEX_LEN - 1], 0);
        assert!(hex[..16].iter().all(|b| b.is_ascii_hexdigit()));
        // SAFETY: `inst` came from `duospool_create` and is destroyed exactly once, here.
        unsafe { duospool_destroy(inst) };
    }

    #[test]
    fn null_arguments_and_null_buffer() {
        // SAFETY: a null `out` is the case under test and is checked before any write.
        assert_eq!(
            unsafe { duospool_create(ptr::null_mut()) },
            Status::ErrNullArg.code()
        );
        let inst = create();
        // SAFETY: `inst` is live; the null buffer is the case under test and is checked before any read.
        let status = unsafe { duospool_load(inst, ptr::null(), CONTENT_SIZE) };
        assert_eq!(status, Status::ErrNullArg.code());
        // SAFETY: a null instance is the case under test and is checked before any use.
        let status = unsafe { duospool_load(ptr::null_mut(), content().as_ptr(), CONTENT_SIZE) };
        assert_eq!(status, Status::ErrNullArg.code());
        // SAFETY: a null instance is the case under test and is checked before any use.
        assert_eq!(
            unsafe { duospool_step(ptr::null_mut(), 1) },
            Status::ErrNullArg.code()
        );
        // SAFETY: `inst` is live; the null output is checked before any write.
        assert_eq!(
            unsafe { duospool_time(inst, ptr::null_mut()) },
            Status::ErrNullArg.code()
        );
        // SAFETY: a null instance is a no-op by contract.
        unsafe { duospool_destroy(ptr::null_mut()) };
        // SAFETY: `inst` came from `duospool_create` and is destroyed exactly once, here.
        unsafe { duospool_destroy(inst) };
    }

    #[test]
    fn short_digest_buffer() {
        let inst = create();
        for out_len in [0_usize, 1, DIGEST_HEX_LEN - 1] {
            let mut buf = vec![0xAA_u8; out_len];
            // SAFETY: `inst` is live; `buf` is valid for `out_len` bytes and is not written, because the length is short.
            let status =
                unsafe { duospool_digest_hex(inst, buf.as_mut_ptr().cast::<c_char>(), out_len) };
            assert_eq!(status, Status::ErrShortBuffer.code());
            assert!(buf.iter().all(|&b| b == 0xAA));
        }
        // SAFETY: `inst` is live; a null output is checked before any write.
        let status = unsafe { duospool_digest_hex(inst, ptr::null_mut(), DIGEST_HEX_LEN) };
        assert_eq!(status, Status::ErrNullArg.code());
        // SAFETY: `inst` came from `duospool_create` and is destroyed exactly once, here.
        unsafe { duospool_destroy(inst) };
    }

    #[test]
    fn refused_length_never_reads_the_buffer() {
        let inst = create();
        let small = [0_u8; 8];
        // The claimed lengths are far larger than the 8-byte allocation. A refused length must be decided from the
        // length alone, so Miri would report an out-of-bounds read if the facade touched the buffer.
        for (len, want) in [
            (0_usize, Status::RefuseEmpty),
            (4, Status::RefuseTruncated),
            (duospool_core::HEADER_SIZE, Status::RefuseHeader),
            (98_304, Status::RefuseBadSize),
            (duospool_core::MAX_SIZE + 1, Status::RefuseOversized),
            (usize::MAX, Status::RefuseOversized),
        ] {
            // SAFETY: `inst` is live; `small` is valid for 8 bytes and is not read for these refused lengths.
            let status = unsafe { duospool_load(inst, small.as_ptr(), len) };
            assert_eq!(status, want.code(), "length {len}");
        }
        // SAFETY: `inst` came from `duospool_create` and is destroyed exactly once, here.
        unsafe { duospool_destroy(inst) };
    }

    #[test]
    fn probes_match_the_extern_paths() {
        assert_eq!(probe::probe_load_null(), Status::ErrNullArg.code());
        assert_eq!(probe::probe_digest_short(16), Status::ErrShortBuffer.code());
    }
}
