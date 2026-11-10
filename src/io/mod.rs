use crate::error::Errno;
use crate::fd::AsFdRef;
use crate::fd::AsRawFd;

pub mod raw;

/// Write data to a file descriptor.
pub fn write<F: AsFdRef>(fd: F, buff: &[u8]) -> Result<usize, Errno> {
    let fd = fd.as_fd_ref().as_raw_fd();
    let res = unsafe { raw::write(fd, buff.as_ptr(), buff.len()) };
    match res {
        Ok(x) => return Ok(x),
        Err(e) => {
            debug_assert!(e > 0);
            return unsafe { Err(Errno::from_raw_unchecked(e)) };
        },
    }
}
