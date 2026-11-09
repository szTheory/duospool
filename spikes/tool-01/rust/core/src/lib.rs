// SPDX-License-Identifier: MIT OR Apache-2.0
// SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors

//! The TOOL-01 spike workload in safe Rust (protocol W-01 to W-07).
//!
//! One `Instance` owns all of its state. There are no mutable globals or statics, no std I/O, no clock, no threads and
//! no hash maps. The null cases of the C API are handled by the capi crate, because a Rust slice cannot be null.
#![forbid(unsafe_code)]

/// Length of the content a load accepts (W-05).
pub const CONTENT_SIZE: usize = 65_536;
/// Length of a header-prefixed image, which is refused and never stripped (W-05).
pub const HEADER_SIZE: usize = 66_048;
/// Largest length that is not refused as oversized (W-05).
pub const MAX_SIZE: usize = 1_048_576;
/// Smallest output buffer for the hex digest: 16 digits plus NUL (W-04).
pub const DIGEST_HEX_LEN: usize = 17;

const FNV_OFFSET: u64 = 0xcbf2_9ce4_8422_2325;
const FNV_PRIME: u64 = 0x0000_0100_0000_01b3;

/// Status values with the fixed numeric codes of W-03.
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Status {
    Ok = 0,
    RefuseEmpty = 1,
    RefuseTruncated = 2,
    RefuseBadSize = 3,
    RefuseOversized = 4,
    RefuseHeader = 5,
    ErrNullArg = 6,
    ErrShortBuffer = 7,
    ErrState = 8,
}

impl Status {
    /// The numeric code of W-03.
    pub const fn code(self) -> u32 {
        self as u32
    }

    /// The status for a numeric code, or `None` for a code outside W-03.
    pub const fn from_code(code: u32) -> Option<Status> {
        match code {
            0 => Some(Status::Ok),
            1 => Some(Status::RefuseEmpty),
            2 => Some(Status::RefuseTruncated),
            3 => Some(Status::RefuseBadSize),
            4 => Some(Status::RefuseOversized),
            5 => Some(Status::RefuseHeader),
            6 => Some(Status::ErrNullArg),
            7 => Some(Status::ErrShortBuffer),
            8 => Some(Status::ErrState),
            _ => None,
        }
    }

    /// The transcript name of the status (protocol transcript format).
    pub const fn name(self) -> &'static str {
        match self {
            Status::Ok => "OK",
            Status::RefuseEmpty => "REFUSE_EMPTY",
            Status::RefuseTruncated => "REFUSE_TRUNCATED",
            Status::RefuseBadSize => "REFUSE_BAD_SIZE",
            Status::RefuseOversized => "REFUSE_OVERSIZED",
            Status::RefuseHeader => "REFUSE_HEADER",
            Status::ErrNullArg => "ERR_NULL_ARG",
            Status::ErrShortBuffer => "ERR_SHORT_BUFFER",
            Status::ErrState => "ERR_STATE",
        }
    }
}

/// The W-05 refusals that depend on the length alone, in decision order. `Status::Ok` means the length is acceptable.
/// A caller that holds only a pointer and a length (the C facade) uses this first, so that no byte is read, and no slice
/// is formed, for a length that will be refused.
pub const fn length_status(len: usize) -> Status {
    if len == 0 {
        Status::RefuseEmpty
    } else if len > MAX_SIZE {
        Status::RefuseOversized
    } else if len == HEADER_SIZE {
        Status::RefuseHeader
    } else if len < CONTENT_SIZE {
        Status::RefuseTruncated
    } else if len != CONTENT_SIZE {
        Status::RefuseBadSize
    } else {
        Status::Ok
    }
}

const fn fold_byte(digest: u64, byte: u8) -> u64 {
    (digest ^ byte as u64).wrapping_mul(FNV_PRIME)
}

/// One machine instance (W-02): all state is owned here.
#[derive(Debug)]
pub struct Instance {
    content: Vec<u8>,
    time: u64,
    digest: u64,
}

impl Default for Instance {
    fn default() -> Self {
        Self::new()
    }
}

impl Instance {
    /// A fresh unloaded instance with the FNV-1a offset basis as its digest (W-07).
    pub const fn new() -> Self {
        Instance {
            content: Vec::new(),
            time: 0,
            digest: FNV_OFFSET,
        }
    }

    /// Whether a load has succeeded.
    pub fn is_loaded(&self) -> bool {
        self.content.len() == CONTENT_SIZE
    }

    /// Load content following the W-05 decision order. Every refusal leaves time and digest unchanged.
    pub fn load(&mut self, bytes: &[u8]) -> Status {
        // Liveness control for the G5 fuzz gate only. Placed before the length ladder, as in the C option, so that the
        // 4-byte input `DSPB` reaches it instead of being refused as truncated. Only the planted fuzz build enables it.
        // The prefix is compared as one integer (little-endian "DSPB"), not through a memcmp call: under the ASan runtime
        // on macOS libFuzzer did not see the bcmp that `starts_with` compiles to, and 60 s found nothing (LOG.md).
        #[cfg(feature = "planted-bug")]
        if let Some(prefix) = bytes.first_chunk::<4>()
            && u32::from_le_bytes(*prefix) == 0x4250_5344
        {
            panic!("planted bug: input starts with DSPB");
        }
        let by_length = length_status(bytes.len());
        if by_length != Status::Ok {
            return by_length;
        }
        if self.is_loaded() {
            return Status::ErrState;
        }
        let mut content = Vec::new();
        if content.try_reserve_exact(CONTENT_SIZE).is_err() {
            return Status::ErrState; // no allocation status exists in W-03
        }
        content.extend_from_slice(bytes);
        let mut digest = self.digest;
        for &byte in &content {
            digest = fold_byte(digest, byte);
        }
        self.digest = digest;
        self.content = content;
        Status::Ok
    }

    /// Add `cycles` to the machine time and fold the 8 little-endian bytes of the new time into the digest (W-06).
    pub fn step(&mut self, cycles: u32) -> Status {
        if !self.is_loaded() {
            return Status::ErrState;
        }
        self.time = self.time.wrapping_add(u64::from(cycles));
        for byte in self.time.to_le_bytes() {
            self.digest = fold_byte(self.digest, byte);
        }
        Status::Ok
    }

    /// The machine time in cycles.
    pub const fn time(&self) -> u64 {
        self.time
    }

    /// The FNV-1a 64 digest (W-07).
    pub const fn digest(&self) -> u64 {
        self.digest
    }

    /// Write 16 lowercase hex digits and a NUL into the start of `out`; `out` needs at least `DIGEST_HEX_LEN` bytes.
    pub fn digest_hex(&self, out: &mut [u8]) -> Status {
        const HEX: &[u8; 16] = b"0123456789abcdef";
        if out.len() < DIGEST_HEX_LEN {
            return Status::ErrShortBuffer;
        }
        for (i, slot) in out.iter_mut().take(16).enumerate() {
            let shift = 60 - 4 * (i as u32);
            *slot = HEX[((self.digest >> shift) & 0xf) as usize];
        }
        out[16] = 0;
        Status::Ok
    }
}
