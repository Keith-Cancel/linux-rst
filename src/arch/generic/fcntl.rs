#![allow(unused)]

use core::ffi::c_int;

pub use no_fmt::*;

// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h
#[rustfmt::skip]
pub mod no_fmt {
    use core::ffi::c_int;

    pub const __O_SYNC:    c_int = 0o4000000;
    pub const __O_TMPFILE: c_int = 0o20000000;

    pub const O_ACCMODE:   c_int = 0o0000003;
    pub const O_RDONLY:    c_int = 0o0000000;
    pub const O_WRONLY:    c_int = 0o0000001;
    pub const O_RDWR:      c_int = 0o0000002;
    pub const O_CREAT:     c_int = 0o0000100;
    pub const O_EXCL:      c_int = 0o0000200;
    pub const O_NOCTTY:    c_int = 0o0000400;
    pub const O_TRUNC:     c_int = 0o0001000;
    pub const O_APPEND:    c_int = 0o0002000;
    pub const O_NONBLOCK:  c_int = 0o0004000;
    pub const O_DSYNC:     c_int = 0o0010000;
    pub const O_ASYNC:     c_int = FASYNC;
    pub const FASYNC:      c_int = 0o0020000;
    pub const O_DIRECT:    c_int = 0o0040000;
    pub const O_LARGEFILE: c_int = 0o0100000;
    pub const O_DIRECTORY: c_int = 0o0200000;
    pub const O_NOFOLLOW:  c_int = 0o0400000;
    pub const O_NOATIME:   c_int = 0o1000000;
    pub const O_CLOEXEC:   c_int = 0o2000000;
    pub const O_SYNC:      c_int = __O_SYNC | O_DSYNC;
    pub const O_PATH:      c_int = 0o10000000;
    pub const O_TMPFILE:   c_int = __O_TMPFILE | O_DIRECTORY;
    pub const O_NDELAY:    c_int = O_NONBLOCK;
}

/// Special file descriptor value to tell open should be relative
/// to the current working directory.
pub const AT_FDCWD: c_int = -100;
/// Do not follow symbolic links
pub const AT_SYMLINK_NOFOLLOW: c_int = 0x100;
/// Flag for [`unlinkat`]
///
/// Remove directory instead of unlinking file.
pub const AT_REMOVEDIR: c_int = 0x200;
/// Follow symbolic links.
pub const AT_SYMLINK_FOLLOW: c_int = 0x400;
