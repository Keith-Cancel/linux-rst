#![allow(nonstandard_style)]
use core::ffi::c_uint;

// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/posix_types.h

pub type mode_t = c_uint;
