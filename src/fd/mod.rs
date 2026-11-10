//! Managed file descriptor types and traits.
use core::ffi::c_int;
use core::mem::ManuallyDrop;

/// An owned file descriptor.
#[repr(C)]
#[derive(Debug)]
pub struct Fd(c_int);

impl Drop for Fd {
    fn drop(&mut self) {
        // Not really a good way to safely handle close errors
        // or report them from drop. So just ignore them.
        // Linux from my understanding makes sure that they
        // get closed anyways.
        let _ = unsafe { crate::io::raw::close(self.0) };
        self.0 = -1;
    }
}

impl AsRawFd for Fd {
    #[inline(always)]
    fn as_raw_fd(&self) -> c_int {
        return self.0;
    }
}

impl FromRawFd for Fd {
    #[inline(always)]
    unsafe fn from_raw_fd(fd: c_int) -> Self {
        return Self(fd);
    }
}

impl IntoRawFd for Fd {
    fn into_raw_fd(self) -> c_int {
        let fd = self.0;
        let _ = ManuallyDrop::new(self);
        return fd;
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
