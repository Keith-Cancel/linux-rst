//! File control functions and constants.
#![allow(clippy::missing_safety_doc)]

use core::ffi::c_char;
use core::ffi::c_int;

use crate::arch::posix_types::mode_t;
use crate::arch::syscall;
use crate::types::ErrResult;

#[rustfmt::skip]
pub use crate::arch::fcntl::*;

/// Opens a file, but relative to a file descriptor of a directory.
pub unsafe fn openat(dirfd: c_int, path: *const c_char, flags: c_int, mode: mode_t) -> ErrResult<c_int> {
    let ret = syscall!(dirfd, path as usize, flags, mode, OPEN_AT) as c_int;
    if ret < 0 {
        return Err(-ret);
    }
    return Ok(ret);
}

/// Unlike the file at `path`, but relative to file descriptor of a directory.
pub unsafe fn unlinkat(dirfd: c_int, path: *const c_char, flags: c_int) -> ErrResult {
    let ret = syscall!(dirfd, path as usize, flags, UNLINK_AT) as c_int;
    if ret < 0 {
        return Err(-ret);
    }
    return Ok(());
}
