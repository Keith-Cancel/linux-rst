//! Raw IO calls
#![allow(clippy::missing_safety_doc)]
use core::ffi::c_int;

use crate::arch::Reg;
use crate::arch::syscall;
use crate::error::ErrResult;

/// Close a file descriptor
pub unsafe fn close(fd: c_int) -> ErrResult {
    let ret = syscall!(fd, CLOSE) as c_int;
    if ret < 0 {
        return Err(-ret);
    }
    return Ok(());
}

/// Read data from a file descriptor.
pub unsafe fn read(fd: c_int, buff: *mut u8, len: usize) -> ErrResult<usize> {
    let ret = syscall!(fd, buff as Reg, len, READ) as isize;
    if ret < 0 {
        return Err((-ret) as c_int);
    }
    return Ok(ret as usize);
}

/// Write data to a file descriptor.
#[inline]
pub unsafe fn write(fd: c_int, buff: *const u8, len: usize) -> ErrResult<usize> {
    let ret = syscall!(fd, buff as Reg, len, WRITE) as isize;
    if ret < 0 {
        return Err((-ret) as c_int);
    }
    return Ok(ret as usize);
}
