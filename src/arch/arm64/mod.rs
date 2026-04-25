mod sys;

pub use sys::*;

use super::generic;
use super::sys_call;

#[allow(unused)]
pub mod errno {
    // No over-rides
    pub use super::generic::errno::*;
}

// https://github.com/torvalds/linux/blob/master/arch/arm64/include/uapi/asm/fcntl.h
#[allow(unused)]
pub mod fcntl {
    use core::ffi::c_int;

    pub use super::generic::fcntl::*;

    // Over-ride the generic consts with arch specific values.
    pub const O_DIRECTORY: c_int = 0o40000;
    pub const O_NOFOLLOW: c_int = 0o100000;
    pub const O_DIRECT: c_int = 0o200000;
    pub const O_LARGEFILE: c_int = 0o400000;

    pub const O_TMPFILE: c_int = __O_TMPFILE | O_DIRECTORY;
}

#[allow(unused)]
pub mod limits {
    // No over-rides
    pub use super::generic::limits::*;
}
