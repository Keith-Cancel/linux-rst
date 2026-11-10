//! Managed file descriptor types and traits.
use core::ffi::c_int;

/// An owned file descriptor.
#[repr(C)]
#[derive(Debug)]
pub struct Fd(c_int);

impl AsRawFd for Fd {
    #[inline(always)]
    fn as_raw_fd(&self) -> c_int {
        return self.0;
    }
}

impl FromRawFd for Fd {
    unsafe fn from_raw_fd(fd: c_int) -> Self {
        return Self(fd);
    }
}

/// Allow extracting the raw file descriptor.
pub trait AsRawFd {
    fn as_raw_fd(&self) -> c_int;
}

/// Consume the instance and return its file descriptor.
pub trait IntoRawFd {
    /// Consumes the object, and then return the file descriptor.
    fn into_raw_fd(self) -> c_int;
}

/// Create a new instance from a file descriptor.
pub trait FromRawFd {
    /// Create a new object from the file descriptor.
    /// #Safety
    /// The file descriptor should be exclusive owned by the object.
    unsafe fn from_raw_fd(fd: c_int) -> Self;
}
